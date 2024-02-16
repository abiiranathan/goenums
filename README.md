# goenums

Package goenums is a Go code generator for creating enums based on a simple sql schema.

The package reads CREATE TYPE ... AS ENUM statements from a sql file and
ALTER TYPE ... ADD VALUE/DROP/RENAME VALUE statements from a sql file and generates Go code.

It's designed in C with a modular approach, and it's easy to use.

## Installation

Download the installers from the [releases page](https://github.com/abiiranathan/goenums/releases).

## Usage

```bash
goenums schema/enum.sql gen/types/enums.go enums
```

The arguments are:

- The path to the sql file
- The path to the generated Go file
- The package name

NB: The intermediate directory if any must exist before running the command.

## Example

```sql
CREATE TYPE sex AS ENUM ('M', 'F');
CREATE TYPE marital_status AS ENUM ('S', 'M', 'D', 'W');

-- multiline types
CREATE TYPE status AS ENUM(
    'active',
    'inactive',
    'suspended',
    'deleted'
);

/* multiline types
with comments
And empty lines
*/

CREATE TYPE role AS ENUM (
    'admin', -- inline comment
    'user', -- inline comment
);

ALTER TYPE role ADD VALUE 'superuser' AFTER 'admin';

-- It even works well with case insensitive commands
-- and with the `if exists` clause
alter type role rename value 'user' to 'regular_user';
alter type role drop value 'superuser';
```

```go
package gotypes

type Role string

const (
	RoleAdmin       Role = "admin"
	RoleRegularUser Role = "regular_user"
)

func (e Role) String() string {
	return string(e)
}

func (e Role) GormDataType() string {
	return "role"
}

func (e Role) ValidValues() []string {
	return []string{
		"admin",
		"regular_user",
	}
}

func (e Role) IsValid() bool {
	for _, v := range e.ValidValues() {
		if string(e) == v {
			return true
		}
	}
	return false
}

type Status string

const (
	StatusActive    Status = "active"
	StatusInactive  Status = "inactive"
	StatusSuspended Status = "suspended"
	StatusDeleted   Status = "deleted"
)

func (e Status) String() string {
	return string(e)
}

func (e Status) GormDataType() string {
	return "status"
}

func (e Status) ValidValues() []string {
	return []string{
		"active",
		"inactive",
		"suspended",
		"deleted",
	}
}

func (e Status) IsValid() bool {
	for _, v := range e.ValidValues() {
		if string(e) == v {
			return true
		}
	}
	return false
}

type MaritalStatus string

const (
	MaritalStatusS MaritalStatus = "S"
	MaritalStatusM MaritalStatus = "M"
	MaritalStatusD MaritalStatus = "D"
	MaritalStatusW MaritalStatus = "W"
)

func (e MaritalStatus) String() string {
	return string(e)
}

func (e MaritalStatus) GormDataType() string {
	return "marital_status"
}

func (e MaritalStatus) ValidValues() []string {
	return []string{
		"S",
		"M",
		"D",
		"W",
	}
}

func (e MaritalStatus) IsValid() bool {
	for _, v := range e.ValidValues() {
		if string(e) == v {
			return true
		}
	}
	return false
}

type Sex string

const (
	SexM Sex = "M"
	SexF Sex = "F"
)

func (e Sex) String() string {
	return string(e)
}

func (e Sex) GormDataType() string {
	return "sex"
}

func (e Sex) ValidValues() []string {
	return []string{
		"M",
		"F",
	}
}

func (e Sex) IsValid() bool {
	for _, v := range e.ValidValues() {
		if string(e) == v {
			return true
		}
	}
	return false
}

```
