import re
import os

# Create the .netrc file
netrc_content = """machine bonkachen.com
login audres_deploy@bonkachen.com
password BTNRH1982!
"""
with open('.netrc', 'w') as f:
    f.write(netrc_content)
os.chmod('.netrc', 0o600)

# Add to .gitignore
gitignore_content = ""
if os.path.exists('.gitignore'):
    with open('.gitignore', 'r') as f:
        gitignore_content = f.read()

if '.netrc' not in gitignore_content:
    with open('.gitignore', 'a') as f:
        f.write('\n.netrc\n')

# Patch Makefiles
for fn in ['Makefile', 'makefile.mac']:
    with open(fn, 'r') as f:
        content = f.read()
    
    # Replace the hardcoded credentials with --netrc-file .netrc
    # The URL changes from ftp://audres_deploy...:BTNRH...@bonkachen.com/...
    # to ftp://bonkachen.com/...
    
    content = content.replace(
        'curl --ftp-create-dirs -v -T VS18/Output/EMAV_Setup.exe "ftp://audres_deploy%40bonkachen.com:BTNRH1982%21@bonkachen.com/',
        'curl --netrc-file .netrc --ftp-create-dirs -v -T VS18/Output/EMAV_Setup.exe "ftp://bonkachen.com/'
    )
    content = content.replace(
        'curl --ftp-create-dirs -v -T VS18/Output/ABRAV_Setup.exe "ftp://audres_deploy%40bonkachen.com:BTNRH1982%21@bonkachen.com/',
        'curl --netrc-file .netrc --ftp-create-dirs -v -T VS18/Output/ABRAV_Setup.exe "ftp://bonkachen.com/'
    )
    content = content.replace(
        'curl --ftp-create-dirs -v -T web/index.html "ftp://audres_deploy%40bonkachen.com:BTNRH1982%21@bonkachen.com/',
        'curl --netrc-file .netrc --ftp-create-dirs -v -T web/index.html "ftp://bonkachen.com/'
    )
    content = content.replace(
        'curl --ftp-create-dirs -v -T EMAV_Mac.zip "ftp://audres_deploy%40bonkachen.com:BTNRH1982%21@bonkachen.com/',
        'curl --netrc-file .netrc --ftp-create-dirs -v -T EMAV_Mac.zip "ftp://bonkachen.com/'
    )
    content = content.replace(
        'curl --ftp-create-dirs -v -T ABRAV_Mac.zip "ftp://audres_deploy%40bonkachen.com:BTNRH1982%21@bonkachen.com/',
        'curl --netrc-file .netrc --ftp-create-dirs -v -T ABRAV_Mac.zip "ftp://bonkachen.com/'
    )
    
    with open(fn, 'w') as f:
        f.write(content)

