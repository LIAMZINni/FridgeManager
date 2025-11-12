#!/bin/bash
# Скрипт выполняется перед удалением пакета
set -e

echo "=========================================="
echo "   Удаление FridgeManager"
echo "=========================================="

# Спрашиваем удалять ли базу данных
if [ "$1" = "remove" ] || [ "$1" = "deconfigure" ]; then
    read -p "❓ Удалить базу данных fridgemanager? [y/N]: " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "🗑️  Удаляем базу данных..."
        sudo -u postgres dropdb --if-exists fridgemanager
        echo "✅ База данных удалена"
    fi
    
    echo "🧹 Очищаем конфигурационные файлы..."
    rm -rf /etc/fridgemanager
    rm -rf /var/lib/fridgemanager
fi

echo "✅ Очистка завершена"