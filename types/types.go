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
