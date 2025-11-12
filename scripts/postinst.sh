#!/bin/bash
set -e

echo "Setting up FridgeManager database..."

# Install PostgreSQL if needed
if ! command -v psql &> /dev/null; then
    echo "Installing PostgreSQL..."
    apt-get update
    apt-get install -y postgresql
fi

# Start PostgreSQL
systemctl start postgresql || true

# Create database
sudo -u postgres createdb fridgemanager 2>/dev/null || echo "Database already exists"

# Create table
sudo -u postgres psql -d fridgemanager -c "CREATE TABLE IF NOT EXISTS products (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) UNIQUE NOT NULL,
    current_quantity INTEGER NOT NULL DEFAULT 0,
    norm_quantity INTEGER NOT NULL
);" || echo "Table already exists"

# Add sample data
sudo -u postgres psql -d fridgemanager -c "INSERT INTO products (name, current_quantity, norm_quantity) VALUES
('Cottage Cheese', 5, 10),
('Cheese', 12, 15),
('Milk', 18, 20),
('Eggs', 25, 30),
('Olives', 3, 8)
ON CONFLICT (name) DO NOTHING;" || echo "Data already exists"

echo "Setup completed! Run: fridgemanager"