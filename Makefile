default : bin

-include local/custom.mk

release :
	-rm -r build
	sake all release
	sake all release kits=s60_30,s60_31 cert=dev

apidoc :
	sake --trace cxxdoc
