# eve-memory-reader

This is mostly a passion project inspired by https://github.com/Arcitectus/Sanderling with the aim of being faster and use more memory efficient.

This project is split into 2 parts:
- eve-memory-reader: written in C, performs all the memory crawling and resolving tasks, returns json-like responses (UI components) to the caller
- eve-bot-framework: written in Python, provides a few things:
  - a high-level entrypoint for bots (written in yaml, and by that logic also json). Check here for [some examples](https://github.com/jamesalbert/eve-memory-reader/tree/main/eve-bot-framework/examples)
  - a UI for loading these bot configurations in


There are also 2 additional folders:
- eve-memory-reader-test: a simple invocation of eve-memory-reader so that we can perform memory leak analysis, code coverage, general testing.
- orchestration: a minimal setup for standing up a windows machine with steam installed using Vagrant and Ansible.


tbd: in-depth documentation
