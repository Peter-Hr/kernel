# Team milestones

This repository will be used for **all team milestones** in the course.

Apart from milestone M02, which is already [present](README-m02.md), the
subsequent milestones will be provided as updates to the
[`upstream/team`](https://gitlab.mff.cuni.cz/teaching/nswi200/2025/upstream/team)
repository. You are required to merge the updates from the upstream repository
into this repository.

Please see the ["Keeping your fork up-to-date"](https://d3s.mff.cuni.cz/teaching/nswi200/qa/#how-to-keep-my-fork-up-to-date)
section in the *Q & A* part of the course website for a quick guide. The only
difference is that you will be using an upstream repository for the team
milestones.

We suggest that you add the upstream repository to your GitLab watch-list to
receive notifications about any activity in that repository.


## Environment

Please note that all team milestones **require the MSIM simulator** and the
corresponding **cross-compiler tool-chain**.

At this point (after milestone M01) everybody should have access to a
working development environment with MSIM -- either on a remote host
(`lab.d3s.mff.cuni.cz:22200` or the *Rotunda lab* machines), or on your own
computer (instructions for local installation are provided on a
[separate page](https://d3s.mff.cuni.cz/teaching/nswi200/toolchain/)).


## Milestones

The tasks for individual milestones are specified in separate files. You will
find the specification of the first team milestone in
[`README-m02.md`](README-m02.md); subsequent milestones will follow the same
naming scheme. The skeleton source code which provides the context for the
milestone is organized in the same way as in milestone M01 (the last
individual milestone), so you should be already familiar with the layout.


## Tests

Just like in the previous (individual) milestones, each team milestone will
come with a suite of tests to help you stay on the right track. The tests
making up each suite are listed in `suites/m*.txt` files, and you can execute
**all tests** simply by typing:

```shell
tools/tester.py suite suites/m*.txt
```

However, for **incremental development and debugging**, we suggest that you
configure your kernel build to execute a single test when you start MSIM. This
will (together with incremental build) speed up the turnaround between code
modification and test execution.

For example, to execute the `basic` test from the `heap` test group when you
start MSIM, you can configure your project as follows:

```shell
make distclean && ./configure.py --debug --kernel-test=heap/basic
```

Then, whenever you modify (and save) your code, you can quickly rebuild the
kernel and run the pre-configured test as follows:

```shell
make && msim
```

Recall that adding the `--debug` flag to the configuration script enables
`dprintk` messages that can help you during debugging.


## CI setup

The `.gitlab-ci.yml` file contains CI configuration for your repository. When
working on tasks from a particular milestone, make sure that you always
execute (and successfully pass) also the test suites from the previous
milestones. Otherwise you might find it difficult to proceed with the actual
milestone.

When you submit a baseline solution, there should be no regressions with
respect to the previous milestones and all tests covering the baseline
solution should be green.


## Submission

To submit your baseline solution, you
[**MUST tag the commit**](https://git-scm.com/book/en/v2/Git-Basics-Tagging)
(on your `master` branch) that represents the state of the repository that you
want evaluated with respect to a specific milestone. The tag name must
correspond to the milestone identifier, i.e., `m02`, `m03`, and so forth.

If there is no tag, your solution will not be considered on time and only
evaluated before the final review at the end of the semester.

### Submission checklist

* `tools/tester.py suite suites/m*.txt` passes (fuzzy tests are not in CI).
* `tools/check_cstyle.sh` passes (part of CI).
* Contributor forms are filled in.
* The right commit is properly tagged.


## Grading

Grading is described in detail in our website. To summarize:

* the baseline solutions for all milestones must be working at the time of
  the final deadline (in mid January)
* a working baseline solution submitted on time (within the soft deadline of
  the respective milestone) earns bonus points
* extended features and code quality are evaluated during final review
  after the final deadline
* [bonus points](https://d3s.mff.cuni.cz/teaching/nswi200/grading/) are awarded
  (or deducted) during final review

We will also provide summary feedback during the final review.
