# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

```bash
# Configure and build
cmake -B build -S .
cmake --build build
```

Project uses CMake 4.1.0+ and C++23. CMake is configured with:
- `CMAKE_CXX_SCAN_FOR_MODULES ENABLE`
- Qt6 automation (`CMAKE_AUTOMOC`, `CMAKE_AUTORCC`, `CMAKE_AUTOUIC`)
- Compile commands export enabled

## Architecture

**SetteiMan** is a Qt6-based animation production management tool organized into modular libraries:

### Core Libraries (from `setman/`)

**SetmanCore** - Production hierarchy and configuration
- `Company` → `Series` → `Episode` three-tier organization structure
- `Series` uses configurable naming conventions with regex-based cut name parsing (see `series.hpp:67-68`)
- `Episode` manages `up_folder` and `cels_folder`, tracks active/archived cuts and generic materials
- Custom `Error` class with severity levels (none/info/warning/error/critical) and typed error codes (see `error.hpp:12-38`)

**SetmanMaterials** - Asset management system
- Base hierarchy: `GenericMaterial` → `File`/`Folder` → specialized types (`Cut`, `Image`, etc.)
- All materials are UUID-tracked via Boost.UUID
- `Cut` extends `Folder`, tracks stage (lo/ka/ls/gs), status progression (not_started→started→in_progress→finishing→done→up), scene/number/take, and maintains status history
- Cut identifiers parsed from filesystem using series naming convention regex
- Materials support tags, aliases, notes, and parent-child relationships
- `Element` represents reusable production elements (characters, backgrounds, props) across a series with aliases and tag support

**SetmanAIEndpoints** - AI service abstraction
- Base classes: `GenericClient`, `GenericRequest`, `GenericResponse`
- Implementations: `GoogleClient` (Vision OCR), `DeeplClient` (translation), `OpenRouterClient`
- All use CURL for HTTP and nlohmann_json for JSON
- Request supports fluent interface for message building and parameter configuration (temperature, max_tokens, top_p/top_k, etc.)

**SetmanTranslationService** - Translation coordination
- Orchestrates AI clients for translation workflows
- Target language support (en/jp)
- Designed for context-aware translation

**SetmanConversations** - Conversation management
- Manages multi-turn conversations with AI services

### UI Layer

**Application** (`qt/`)
- `main.cpp`: Entry point
- `MainWindow`: Primary Qt interface integrating all services

## Key Architectural Patterns

**Hierarchical organization**: Company > Series > Episode mirrors animation production structure. Series defines naming conventions that Episodes use to parse cut folders.

**Materials as folders/files**: Cuts are folders containing child materials (keyframes, images, notes). Parent-child relationships tracked via UUID references.

**Cut progression**: Cuts move through production stages (lo→ka→ls→gs) with status tracking. Archived cuts are moved via `Episode::up_cut()`.

**Tag system**: Both materials and elements support tags for organization. Episodes and Series maintain `tag_lookup_` maps for fast material queries by tag.

**Elements as shared references**: Elements (characters, backgrounds, props) are series-level entities that materials can reference. Supports aliases for name variations.

**Type-safe errors**: `Error` class combines severity, error code enum, and optional custom message. Used with `std::expected` for error propagation.

## Dependencies

Boost (UUID), spdlog, nlohmann_json, CURL, Qt6 (Widgets, Core, Gui)

## User Preferences

- please just give diffs
- keep edits short so i can ask questions
