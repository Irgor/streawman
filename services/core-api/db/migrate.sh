#!/bin/bash
set -e

set -a
source .env
set +a

docker exec -i -e PGPASSWORD="${POSTGRES_PASSWORD}" postgres psql -U "${POSTGRES_USER}" -d "${POSTGRES_DB}" <<EOF
CREATE TABLE IF NOT EXISTS schema_migrations (
  filename TEXT PRIMARY KEY,
  applied_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);
EOF

for file in services/core-api/db/migrations/*.sql; do
    filename=$(basename "$file")
    echo "Processing $filename"

    applied=$(docker exec -i -e PGPASSWORD="${POSTGRES_PASSWORD}" postgres psql -U "${POSTGRES_USER}" -d "${POSTGRES_DB}" -tAc "SELECT 1 FROM schema_migrations WHERE filename='$filename'")
    if [ "$applied" = "1" ]; then
        echo "$filename: Already applied, skipping"
        continue
    fi

    docker exec -i -e PGPASSWORD="${POSTGRES_PASSWORD}" postgres psql -v ON_ERROR_STOP=1 -U "${POSTGRES_USER}" -d "${POSTGRES_DB}" < "$file"
    echo "$filename: Applied successfully"

    docker exec -i -e PGPASSWORD="${POSTGRES_PASSWORD}" postgres psql -v ON_ERROR_STOP=1 -U "${POSTGRES_USER}" -d "${POSTGRES_DB}" <<EOF
        INSERT INTO schema_migrations (filename, applied_at) VALUES ('$filename', CURRENT_TIMESTAMP);
EOF
done

echo "Migration completed successfully"