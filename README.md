Splashscreen-Clipboard
======================

A splash screen application. Currently just for for node-webkit (or any other app called nw.exe.)
Calls the Main-App (nw.exe) in a Child-Process and waits for changes in the Main-App.
The Communication-channel is the clipboard.
Those changes must be triggered from the Main-App with a special Tag:

SPLASH: {progress: 'XX', message: 'Message that will be displayed above the progressbar'}


Current project state is really basic and just supported for
- Windows XP + .Net 4.0
- Windows Vista/7/8/8.1

just Windows XP needs .Net 4.0
If someone can tell me why, you're welcome :-)

The project-files are used with Visual Studio 2012, but the compilation is configured for Visual C++ 10.

