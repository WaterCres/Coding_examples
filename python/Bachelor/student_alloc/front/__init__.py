from django.db.backends.signals import connection_created

def activate_wal_mode(sender, connection, **kwargs):
    """Enable Write ahead logging in sqlite."""
    if connection.vendor == 'sqlite':
        cursor = connection.cursor()
        cursor.execute('PRAGMA journal_mode=WAL;')

connection_created.connect(activate_wal_mode)