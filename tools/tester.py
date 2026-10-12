#!/usr/bin/env python3

# SPDX-License-Identifier: Apache-2.0
# Copyright 2019 Charles University


import argparse
import json
import logging
import multiprocessing
import os
import re
import shutil
import subprocess
import sys
import threading
import time

CPU_COUNT = multiprocessing.cpu_count()


class TesterException(Exception):
    def __init__(self, message, details):
        Exception.__init__(self, message)
        self.details = details

class UnreachableCode(TesterException):
    def __init__(self):
        import inspect
        info = inspect.getframeinfo(inspect.stack()[1][0])
        position = '{}:{}'.format(info.filename, info.lineno)
        TesterException.__init__(self,
                                 'internal error at {}'.format(position),
                                 'reached unreachable code :-(')

class InThreadPopener(threading.Thread):
    def __init__(self, command, work_dir, log_filename, logger, stdin=subprocess.DEVNULL):
        threading.Thread.__init__(self)
        self.command = command
        self.work_dir = work_dir
        self.log_filename = log_filename
        self.logger = logger
        self.stdin = stdin
        self.output = []
        self.exit_code = None
        self.proc = None
        self.last_exception = None

    def run(self):
        try:
            with open(self.log_filename, 'wt') as log_file:
                self.proc = subprocess.Popen(self.command,
                                             stdin=self.stdin,
                                             stdout=subprocess.PIPE,
                                             stderr=subprocess.STDOUT,
                                             cwd=self.work_dir)
                for line in self.proc.stdout:
                    line = line.decode('utf-8', errors='backslashreplace')
                    line2 = line.rstrip()
                    self.output.append(line2)
                    self.logger.debug('[%s] %s', self.command[0], line2)
                    log_file.write(line)
            self.proc.wait()
            self.exit_code = self.proc.returncode
        except Exception as exc:
            self.last_exception = exc

    def forceful_wait(self, timeout):
        self.join(timeout)
        if self.is_alive():
            self.logger.error('Forcefully killing %s', self.command)
            self.exit_code = -1
            self.forcefully_terminate()
            self.join(1)
            if self.is_alive():
                self.logger.critical('Failed to kill %s', self.command)
            raise TesterException('{} timed-out'.format(self.command[0]),
                                  'timed-out after {} seconds'.format(timeout))

    def forcefully_terminate(self):
        for _ in range(5):
            try:
                if self.proc.poll() is not None:
                    break
                self.proc.terminate()
                if self.proc.poll() is not None:
                    break
                time.sleep(1)
                if self.proc.poll() is not None:
                    break
                self.proc.kill()
            except OSError as exc:
                self.logger.debug(exc)
                # Probably (fingers crossed) a race between poll() and kill()

def run_command_with_logging(command, work_dir, log_filename, logger, stdin=subprocess.DEVNULL, timeout=600):
    proc = InThreadPopener(command, work_dir, log_filename, logger, stdin)
    proc.start()
    proc.forceful_wait(timeout)
    if proc.last_exception:
        raise TesterException('execution of {} failed'.format(command[0]),
                              str(proc.last_exception))
    return (proc.exit_code, proc.output)

def prepare_empty_build_dir(test_name):
    test_name_clean = re.sub(r'[^\w\-_]', '_', test_name.replace('/', '__'))
    dir_name = os.path.join(os.getcwd(), 'target', test_name_clean)
    try:
        shutil.rmtree(dir_name)
    except FileNotFoundError:
        pass

    os.makedirs(dir_name)
    return dir_name

def parse_test_config(test_descriptor):
    parts = test_descriptor.split(':')
    name = parts[0]

    extras = {
        'memory_size': None,
        'kernel_cflags': [],
    }

    for part in parts[1:]:
        if part.startswith('m'):
            extras['memory_size'] = int(part[1:])
        elif part.startswith('kp'):
            (head, tail) = list(int(s) for s in part[2:].split('-'))
            extras['kernel_cflags'].append(f'KERNEL_PADDING_HEAD={head}')
            extras['kernel_cflags'].append(f'KERNEL_PADDING_TAIL={tail}')
        elif part.startswith('D'):
            extras['kernel_cflags'].append(part[1:])
        else:
            raise TesterException(
                'initialization failed',
                'unknown directive {}'.format(part)
            )

    return ( name, extras )

def run_os_test(test_descriptor, extra_configure_args, logger, build_dir_name):
    ( name, parameters ) = parse_test_config(test_descriptor)
    build_dir = prepare_empty_build_dir(build_dir_name)
    logger.debug('Will use build directory %s.', build_dir)

    configure_args = []
    configure_args.extend(extra_configure_args)

    if parameters['memory_size'] is not None:
        mem_size = parameters['memory_size']
        configure_args.append(f'--memory-size={mem_size}')

    for val in parameters['kernel_cflags']:
        configure_args.append(f'--kernel-cflags={val}')

    logger.info('Configuring (%s)...', ' '.join(configure_args))
    (exit_code, _) = run_command_with_logging(
        ['../../configure.py', '--verbose'] + configure_args,
        build_dir,
        os.path.join(build_dir, 'configure.log'),
        logger,
        timeout=60,
    )
    if exit_code != 0:
        raise TesterException('configuration failed', 'see configure.log')

    logger.info('Building ...')
    (exit_code, _) = run_command_with_logging(
        ['make'],
        build_dir,
        os.path.join(build_dir, 'make.log'),
        logger,
        timeout=120,
    )
    if exit_code != 0:
        raise TesterException('build failed', 'see make.log')

    msim_log = os.path.join(build_dir, 'msim.log')

    logger.info('Running MSIM ...')
    (exit_code, output) = run_command_with_logging(
        ['msim'],
        build_dir,
        msim_log,
        logger,
        timeout=240,
    )
    if exit_code != 0:
        raise TesterException('MSIM execution failed', 'see msim.log')

    # Determine test result
    should_panic = False
    actually_panicked = False
    found_test_passed = False

    for line in output:
        if line == '[ ENDS WITH PANIC ]':
            should_panic = True
        if 'Kernel panic:' in line:
            actually_panicked = True
            break
        if line == 'Test finished.':
            found_test_passed = True
            break

    if should_panic:
        if not actually_panicked:
            raise TesterException('test failed (did not panic)', 'see msim.log')

    test_passed_bad = found_test_passed and should_panic
    test_failed_bad = (not found_test_passed) and (not should_panic)
    if test_passed_bad or test_failed_bad:
        raise TesterException('test failed', 'see msim.log')

    logger.info('Checking test output ...')
    (exit_code, _) = run_command_with_logging(
        ['../../tools/check_output.py'],
        build_dir,
        os.path.join(build_dir, 'report.log'),
        logger,
        stdin=open(msim_log, 'r'),
        timeout=30,
    )

    if exit_code != 0:
        raise TesterException('test failed', 'see report.log')

def run_kernel_test(test_descriptor, extra_configure_args):
    ( name, _ ) = parse_test_config(test_descriptor)
    configure_args = [f'--kernel-test={name}']
    configure_args.extend(extra_configure_args)
    run_os_test(test_descriptor,
                configure_args,
                logging.getLogger('K/{}'.format(name)),
                'kernel/{}'.format(test_descriptor))

def get_suite_tests(suite_filename):
    with open(suite_filename, 'r') as suite:
        for line in suite:
            line = line.strip()
            if line.startswith('#') or (line == ''):
                continue
            parts = line.split()
            if len(parts) != 2:
                raise TesterException('invalid suite file {}'.format(suite_filename),
                                      'invalid line {}'.format(line))
            yield (parts[0], parts[1])


def run_one_test(test, logger, report, extra_configure_args, completed_cond=None):
    logger.info("Starting test %s ...", test['name'])
    try:
        if test['type'] == 'kernel':
            run_kernel_test(test['name'], extra_configure_args)
        else:
            raise TesterException('unknown test type {} for {}'.format(
                test['type'], test['name']), '')

        report.append({
            'type': test['type'],
            'name': test['name'],
            'status': 'passed'
        })

    except TesterException as exc:
        logger.error('Test %s failed: %s (%s).', test['name'], exc, exc.details)
        report.append({
            'type': test['type'],
            'name': test['name'],
            'status': 'failed',
            'message': str(exc),
        })

    except Exception as exc:
        # This is an unexpected exception, probably a bug in tester itself.
        # We print as much info as possible.
        logger.error('Test %s failed: %s.', test['name'], exc, exc_info=exc)
        report.append({
            'type': test['type'],
            'name': test['name'],
            'status': 'error',
            'message': "{} thrown: {}".format(exc.__class__.__name__, str(exc)),
        })

    if completed_cond:
        completed_cond.acquire()
        completed_cond.notify()
        completed_cond.release()


def run_tests(tests, fail_fast, extra_configure_args, concurrent_job_count):
    logger = logging.getLogger('run_tests')

    if concurrent_job_count > 1:
        with multiprocessing.Manager() as manager:
            report = manager.list()
            completed_cond = manager.Condition()
            completed_cond.acquire()

            with multiprocessing.Pool(concurrent_job_count) as pool:
                for test in tests:
                    logger.debug("Submitting test %s", test)
                    pool.apply_async(
                        run_one_test,
                        (test, logger, report, extra_configure_args, completed_cond)
                    )

                while True:
                    completed_cond.wait()
                    if (len(report) == len(tests)):
                        break
                    if fail_fast and any(i['status'] != 'passed' for i in report):
                        logger.warning('Fail fast turned on, terminating immediately.')
                        pool.terminate()
                        break

                completed_cond.release()

                pool.close()
                pool.join()

            return list(report)

    else:
        # Fallback: run as before
        report = []
        for test in tests:
            run_one_test(test, logger, report, extra_configure_args)
            if (report[-1]['status'] != 'passed') and fail_fast:
                logger.warning('Fail fast turned on, terminating immediately.')
                break

        return report


def all_tests_passed(report):
    for result in report:
        if result['status'] != 'passed':
            return False
    return True


def print_report(report, json_filename):
    logger = logging.getLogger('report')
    logger.info('')
    logger.info('Test run report:')
    logger.info('')

    count_passed = 0
    count_total = 0
    for result in report:
        if result['status'] == 'passed':
            logger.info(' - %s passed', result['name'])
            count_passed = count_passed + 1
        elif result['status'] == 'failed':
            logger.info(' - %s FAILED (%s)', result['name'], result['message'])
        else:
            raise UnreachableCode()

        count_total = count_total + 1

    logger.info('')
    count_failures = count_total - count_passed
    if count_failures == 0:
        logger.info('All tests passed.')
    else:
        logger.info('There were failures: %d passed, %d failed.', count_passed, count_failures)

    if json_filename:
        with open(json_filename, 'wt', encoding='UTF-8') as f:
            print(json.dumps({
               'counts': {
                   'total': count_total,
                   'passed': count_passed,
                   'failed': count_failures,
               },
               'tests': report,
            }, indent=4), file=f)

def print_report_by_suite(json_report, suites):
    STATUS_IN_BRACKETS = {
        "passed": "  OK  ",
        "failed": " FAIL ",
    }
    results = {}

    with open(json_report, 'rt', encoding='UTF-8') as f:
        report = json.load(f)
        for t in report.get('tests', []):
            results[t['name']] = t

    for suite in suites:
        print(suite)
        for (test_type, test_name) in get_suite_tests(suite):
            res = results.get(test_name, {"type": test_type, "status": "unknown", "message": "test not executed"})
            suffix = "" if res["status"] == "passed" else " ({})".format(res["message"])
            print("    [{status}] {type} {name}{suffix}".format(
                status=STATUS_IN_BRACKETS.get(res["status"], "  ??  "),
                type=test_type,
                name=test_name,
                suffix=suffix
            ))

def main():
    common_args = argparse.ArgumentParser(add_help=False)
    common_args.add_argument('--verbose',
                             default=False,
                             dest='verbose',
                             action='store_true',
                             help='Be verbose.')
    common_args.add_argument('--fail-fast',
                             default=False,
                             dest='fail_fast',
                             action='store_true',
                             help='Stop execution on first failed test.')
    common_args.add_argument('-j',
                             dest='parallelism',
                             default=1,
                             const=CPU_COUNT,
                             nargs='?',
                             type=int,
                             help='Run tests in parallel (experimental).')
    common_args.add_argument('--json-report',
                             default=None,
                             dest='json_report',
                             metavar='FILENAME',
                             help='Store test results in a JSON file.')
    common_args.add_argument('--configure-arg',
                             dest='configure_args',
                             default=[],
                             action='append',
                             help='Extra argument for configure.py.')

    args = argparse.ArgumentParser(description='Run NSWI200 tests')
    args.set_defaults(action='help')
    args.set_defaults(logging_level=logging.INFO)

    args_sub = args.add_subparsers(help='Select what to do.')

    args_help = args_sub.add_parser('help', help='Show this help.')
    args_help.set_defaults(action='help')

    args_kernel = args_sub.add_parser('kernel', help='Run kernel test.', parents=[common_args])
    args_kernel.set_defaults(action='kernel')
    args_kernel.add_argument('test_names',
                             metavar='TEST_NAME',
                             nargs='+',
                             help='Kernel test names.')

    args_suite = args_sub.add_parser('suite', help='Run whole test suite.', parents=[common_args])
    args_suite.set_defaults(action='suite')
    args_suite.add_argument('suite_files',
                            metavar='SUITE_DESCRIPTOR_FILE',
                            nargs='+',
                            help='Filename with test suite description.')

    args_report = args_sub.add_parser('report', help='Nicely format results of previous run.')
    args_report.set_defaults(verbose=False)
    args_report.set_defaults(action='report')
    args_report.add_argument('report_file',
                             metavar='JSON_REPORT_FILENAME',
                             help='Filename (JSON results from a previous run).')
    args_report.add_argument('suite_files',
                             metavar='SUITE_DESCRIPTOR_FILE',
                             nargs='+',
                             help='Filename with test suite description.')

    if len(sys.argv) < 2:
        class HelpConfig:
            def __init__(self):
                self.action = 'help'
        config = HelpConfig()
    else:
        config = args.parse_args()

    if config.action == 'help':
        args.print_help()
        return 0

    config.logging_level = logging.DEBUG if config.verbose else logging.INFO
    logging.basicConfig(format='[%(asctime)s %(name)-25s %(levelname)7s] %(message)s',
                        level=config.logging_level)

    if config.action == 'report':
        print_report_by_suite(config.report_file, config.suite_files)
        return 0

    tests = []
    if config.action == 'kernel':
        for test in config.test_names:
            tests.append({
                'type': 'kernel',
                'name': test
            })
    elif config.action == 'suite':
        for suite in config.suite_files:
            for (test_type, test_name) in get_suite_tests(suite):
                tests.append({
                    'type': test_type,
                    'name': test_name
                })
    else:
        raise UnreachableCode()

    report = run_tests(tests, config.fail_fast, config.configure_args, config.parallelism)

    print_report(report, config.json_report)

    return 0 if all_tests_passed(report) else 1


if __name__ == "__main__":
    sys.exit(main())
