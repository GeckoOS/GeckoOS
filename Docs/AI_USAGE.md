# AI Usage
This document details rules regarding the usage of AI in the development of GeckoOS.

## Rules
- No autonomous AI/Agentic AI
    - Includes openclaw, claude code, cursor, etc.

- Every line of code must be human reviewed
    - Said human reviewer must also fully understand all AI generated code.

- No more than 50% of a PR should be AI generated, you must still write code manually

- All code must be fully tested

## Exent of AI usage
- IDE Autocomplete
- Web based chat interfaces
- CLI based chat interfaces

## What usage of AI is intended for
- Debugging code
- Planning refactors
- Research
- Coding repetitive tasks that make no sense for a human to code

## What AI usage is not intended for
- Coding critical parts of the codebase
- Slopping an entire PR
- Cranking out 500 features an hour
- Covering up your disallowed AI usage

## Other Requirements
If you used AI in a PR in any form, you must add in your commit message the exent of AI usage, the way you used AI, and what provider and model was used. For example: `Added feature. Light debugging assistance from Claude Opus 6.7`