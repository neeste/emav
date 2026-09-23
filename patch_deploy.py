import sys

for fn in ['Makefile', 'makefile.mac']:
    with open(fn, 'r') as f:
        content = f.read()
    content = content.replace('deploy:\n', 'deploy: mac_deploy mac_deploy_abrav\n')
    with open(fn, 'w') as f:
        f.write(content)
