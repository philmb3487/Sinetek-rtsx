
sudo kextunload -b com.sinet3k.Sinetek-rtsx

xcodebuild -configuration Debug &&
sudo cp -R build/Debug/Sinetek-rtsx.kext /tmp &&
sudo chown -R root:wheel /tmp/Sinetek-rtsx.kext &&
sudo kextutil /tmp/Sinetek-rtsx.kext;

