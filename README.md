# eve-memory-reader

This is mostly a passion project inspired by https://github.com/Arcitectus/Sanderling with the aim of being faster and more memory efficient.

This project is split into 2 parts:
- eve-memory-reader: written in C, performs all the memory crawling and resolving tasks, returns json-like responses (UI components) to the caller
- eve-bot-framework: written in Python, provides a few things:
  - a high-level entrypoint for bots (written in yaml, and by that logic also json). Check here for [some examples](https://github.com/jamesalbert/eve-memory-reader/tree/main/eve-bot-framework/examples)
  - a UI for loading these bot configurations in
  - send you SMS messages when certain events occur


There are also 2 additional folders:
- eve-memory-reader-test: a simple invocation of eve-memory-reader so that we can perform memory leak analysis, code coverage, general testing.
- orchestration: a minimal setup for standing up a windows machine with steam installed using Vagrant and Ansible.

## Installation

```
# build the dll
nuget restore .
msbuild /m /p:Configuration=Release .

# build the exe
pip install -r eve-bot-framework/requirements.txt
pyinstaller --clean --onefile --name="eve-bot-application" --paths ".\eve-bot-framework" --add-data=".\x64\Release\eve-memory-reader.dll;." .\eve-bot-framework\app\app.py
```

You should now be able to run `./dist/eve-bot-application.exe` and see two windows open:

![Screenshot 2023-03-07 115619](https://user-images.githubusercontent.com/1617698/223538293-a3953ad4-c8ed-4ecb-b619-35ddf89678d8.png)

It should automatically detect the Eve process if it's running.

The flask server is used to capture pause, stop, and healthcheck events that can interrupt the execution of a bot.

The UI window is used for end-user ease of use. Click browse, use one of the examples to get started.

That's pretty much it. When you start the bot, it will switch to the Eve window and perform actions.

tbd: in-depth documentation
