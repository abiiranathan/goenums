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