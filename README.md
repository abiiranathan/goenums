# goenums

Package goenums is a Go code generator for creating enums based on a simple sql schema.

The package reads CREATE TYPE ... AS ENUM statements from a sql file and
ALTER TYPE ... ADD VALUE/DROP/RENAME VALUE statements from a sql file and generates Go code.

It's designed in C with a modular approach, and it's easy to use.

## Installation

Download the installers from the [releases page](https://github.com/abiiranathan/goenums/releases).
