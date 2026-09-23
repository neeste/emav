
deploy:
	curl --ftp-create-dirs -v -T VS18/Output/EMAV_Setup.exe "ftp://audres_deploy%40bonkachen.com:BTNRH1982%21@bonkachen.com/downloads/EMAV_Setup.exe"
	curl --ftp-create-dirs -v -T VS18/Output/ABRAV_Setup.exe "ftp://audres_deploy%40bonkachen.com:BTNRH1982%21@bonkachen.com/downloads/ABRAV_Setup.exe"

deploy-web:
	curl --ftp-create-dirs -v -T web/index.html "ftp://audres_deploy%40bonkachen.com:BTNRH1982%21@bonkachen.com/rc/emav/index.html"

tag:
	@VER=$$(awk '/#define VERSION /{print $$5}' emav/version.h | tr -d '",') ; \
	if git rev-parse "v$$VER" >/dev/null 2>&1; then \
		echo "Tag v$$VER already exists." ; \
	else \
		echo "Tagging git with v$$VER" ; \
		git tag "v$$VER" ; \
	fi

mac_deploy: emav/emav
	cd emav && ./build_mac_app_pkg.sh
	rm -f EMAV_Mac.zip
	zip -j EMAV_Mac.zip emav/EMAV_App.pkg emav/README.txt
	curl --ftp-create-dirs -v -T EMAV_Mac.zip "ftp://audres_deploy%40bonkachen.com:BTNRH1982%21@bonkachen.com/downloads/EMAV_Mac.zip"

mac_deploy_abrav: abrav/abrav
	rm -f ABRAV_Mac.zip
	zip -j ABRAV_Mac.zip abrav/abrav
	curl --ftp-create-dirs -v -T ABRAV_Mac.zip "ftp://audres_deploy%40bonkachen.com:BTNRH1982%21@bonkachen.com/downloads/ABRAV_Mac.zip"
