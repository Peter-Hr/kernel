#!/usr/bin/env python3

# SPDX-License-Identifier: Apache-2.0
# Copyright 2025 Charles University


import argparse
import glob
import json
import logging
import os
import shlex
import subprocess
import sys

COMPONENTS = {
    'kernel': {
        'c_flags_ignored': [
            '-mno-riscv-attribute',
        ],
        'c_sources_glob': True,
    }
}

def quote_command(args):
    return ' '.join([shlex.quote(i) for i in args])

def get_toolchain_configuration(repo_root, extra_args):
    configure_path = os.path.join(repo_root, 'configure.py')
    configure_args = [
        configure_path,
        '--print-paths-only',
    ] + extra_args

    proc = subprocess.run(configure_args, capture_output=True, check=False)
    if proc.returncode != 0:
        logging.fatal(
            "Running %s ended with exit code %d, aborting!\nStandard output:\n%s\nStandard error output:\n%s",
            quote_command(configure_args),
            proc.returncode,
            proc.stdout.decode(errors='replace'),
            proc.stderr.decode(errors='replace')
        )
        sys.exit(1)
    try:
        return json.loads(proc.stdout)
    except json.JSONDecodeError as e:
        logging.fatal(
            "Invalid output from %s, aborting (%s).\nStandard output:\n%s\nStandard error output:\n%s",
            quote_command(configure_args),
            e,
            proc.stdout.decode(errors='replace'),
            proc.stderr.decode(errors='replace')
        )
        sys.exit(1)

def run_self_describe_(repo_root, component_path):
    make_args = [
        'make',
        '--quiet',
        '-C',
        os.path.join(repo_root, component_path),
        'self-describe',
    ]
    proc = subprocess.run(make_args, capture_output=True, text=True, check=False)
    if proc.returncode != 0:
        logging.fatal(
            "Running %s ended with %d, aborting!",
            quote_command(make_args),
            proc.returncode
        )
        sys.exit(1)
    for line in map(lambda x: x.strip(), proc.stdout.split("\n")):
        if line.startswith('#') or line == '':
            continue
        parts = line.split(" ", 1)
        if len(parts) != 2:
            logging.fatal(
                "Unexpected line in %s: %s",
                quote_command(make_args),
                line
            )
            sys.exit(1)

        yield parts[0], parts[1]

def get_component_configuration_(repo_root, component_path, component_overrides):
    for key, value in run_self_describe_(repo_root, component_path):
        if key == 'SOURCES':
            c_sources = []
            as_sources = []
            for filename in value.split():
                if filename.endswith('.c'):
                    c_sources.append(filename)
                elif filename.endswith('.S'):
                    as_sources.append(filename)
                else:
                    logging.warning("Unsupported source file %s ignored.", filename)
            if component_overrides.get('c_sources_glob', False):
                root_path = os.path.join(repo_root, component_path)
                c_sources = glob.glob("**/*.c", root_dir=root_path, recursive=True)
            yield 'c_sources', c_sources
            yield 'as_sources', as_sources
        elif key == 'CFLAGS':
            flags = value.split()
            for ign in component_overrides.get('c_flags_ignored', []):
                try:
                    flags.remove(ign)
                except ValueError:
                    pass
            yield 'c_flags', flags
        else:
            yield key, value

def get_component_configuration(repo_root, component_path, component_overrides):
    defaults = {
        'c_sources': [],
        'as_sources': [],
        'c_flags': []
    }
    actual = dict(get_component_configuration_(repo_root, component_path, component_overrides))
    return {
        **defaults,
        **actual
    }


def generate_config(repo_root, paths):
    cc = paths['toolchain']['target'] + '-gcc'
    cc_path = os.path.join(paths['toolchain']['directory'], 'bin', cc)

    for component in COMPONENTS:
        info = get_component_configuration(repo_root, component, COMPONENTS[component])
        for src in info['c_sources']:
            target = os.path.join('build', src + '.o')
            yield {
                'directory': os.path.join(repo_root, component),
                'file': src,
                'output': target,
                'arguments': [
                    cc_path,
                ] + info['c_flags'] + [
                    '-c',
                    '-o',
                    target
                ]
            }


def main():
    args = argparse.ArgumentParser(description='Generate IDE configuration')
    args.add_argument('--verbose',
        default=False,
        dest='verbose',
        action='store_true',
        help='Be verbose.'
    )
    args.add_argument(
        '--configure-arg',
        dest='configure_args',
        default=[],
        action='append',
        help='Extra argument for configure.py.'
    )
    args.add_argument(
        '-n', '--dry-run',
        dest='dry_run',
        action='store_true',
        help='Do not create any configuration files but dump to stdout only.'
    )

    config = args.parse_args()

    config.logging_level = logging.DEBUG if config.verbose else logging.INFO
    logging.basicConfig(format='[%(asctime)s %(name)-8s %(levelname)7s] %(message)s',
                        level=config.logging_level)

    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
    logging.debug("Found repo root at %s", repo_root)

    paths = get_toolchain_configuration(repo_root, config.configure_args)
    logging.debug("Toolchain configuration is %s", paths)

    if (not paths['toolchain']['target']) or (not paths['toolchain']['directory']):
        logging.fatal("Toolchain not detected by configure.py, aborting.")
        sys.exit(1)

    compile_commands = list(generate_config(repo_root, paths))

    if config.dry_run:
        logging.info("Dry run enabled, not overwriting anything.")
        logging.info("compile_commands.json:\n%s", json.dumps(compile_commands, sort_keys=True))
        sys.exit(0)

    with open(os.path.join(repo_root, "compile_commands.json"), "w") as f:
        json.dump(compile_commands, f, sort_keys=True, indent=4)

    return 0


if __name__ == "__main__":
    sys.exit(main())
