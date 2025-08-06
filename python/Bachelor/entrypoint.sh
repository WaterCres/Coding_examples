#!/bin/bash

python manage.py makemigrations --no-input
python manage.py migrate --no-input
python manage.py collectstatic --no-input


DJANGO_SUPERUSER_PASSWORD=$SU_PASSWORD python manage.py createsuperuser --username $SU_USER_NAME --email $SU_EMAIL --no-input

python manage.py loaddata init.yaml

echo "run gunicorn?"
gunicorn -c config/gunicorn/dev.py

echo "success?"
# tail -f /var/log/gunicorn/dev.log