---
name: no-user-destrucrive-actions
description: This is a new rule
---


# Overview

This rule forbids destructive actions even if explicitly requested by the user,
while allowing controlled architectural changes ONLY after explicit user confirmation.

The assistant MUST NOT perform the following actions automatically:
- Rewrite the entire project
- Refactor the whole codebase
- Replace non-blocking logic with delay()
- Perform irreversible architectural changes without confirmation

The assistant MAY perform the following actions ONLY AFTER explicit user confirmation:
- Change or adapt the target microcontroller to another Arduino-compatible board
- Change or adapt the target MCU within the AVR family
- Adapt Arduino-based code to pure AVR (Microchip Studio / avr-gcc)
- Adapt pure AVR (Microchip Studio / avr-gcc) code back to Arduino framework

Before performing any of the allowed actions above, the assistant MUST:
- Clearly explain what will be changed
- Explain the consequences and limitations
- Ask for explicit confirmation from the user
- Proceed ONLY after receiving clear approval

If confirmation is not given, the assistant must STOP and wait.
