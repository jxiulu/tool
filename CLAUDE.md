# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

```bash
# Configure and build
cmake -B build -S .
cmake --build build

# The project uses CMake 4.1.0+ and C++23
```

## Architecture

**SetteiMan** is a Qt6-based animation production management tool with three main layers:

### Core Domain (`settei_man/`)
- **Company → Series → Episode hierarchy**: Organization structure mirroring animation production
- **Materials system**: UUID-tracked assets (cuts, keyframes, images) with parent-child relationships
- **Cut tracking**: Manages animation cuts through stages (LO, KA, LS, GS) with status progression
- **Error handling**: Custom `Error` class with severity levels and error codes (see `types.hpp`)

### Web Services (`web/`)
- **AI client abstraction**: `GenericClient` and `GenericRequest` base classes for AI services
- **Implementations**: OpenRouter, Google Vision (OCR), DeepL translation
- All use CURL for HTTP, nlohmann_json for JSON parsing

### UI Layer (`qt/`)
- **MainWindow**: Primary interface for project management and OCR operations
- Integrates Google Vision client for OCR on keyframes

## Key Concepts

- **Materials hierarchy**: `GenericMaterial` → `Matfile`/`Matfolder` → specific types (Cut, Image, Keyframe)
- **Cut naming**: Parsed via regex from folder names using Series naming conventions
- **UUID tracking**: All materials have unique IDs for cross-referencing
- **Episode structure**: Contains active_cuts, archived_cuts, and generic materials

## Dependencies

Boost (UUID), spdlog, nlohmann_json, CURL, Qt6 (Widgets, Core, Gui)

## User Preferences

- please just give diffs
- keep edits short so i can ask questions
