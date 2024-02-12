from subprocess import Popen, PIPE, STDOUT

commands = [
    'insert 1 alice alice@google.com',
    'insert 2 bob bob@yahoo.com',
    'select',
    '.exit'
]

db = Popen(['../build/src/dbtoy'], stdin=PIPE, stdout=PIPE, stderr=STDOUT)
out, err = db.communicate('\n'.join(commands).encode('utf-8'))
print(out.decode())