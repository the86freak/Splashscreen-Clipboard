Splashscreen-Clipboard
======================

A splash screen application. Currently just for for node-webkit (or any other app called nw.exe.)
Calls the Main-App (nw.exe) in a Child-Process and waits for changes in the Main-App.
The Communication-channel is the clipboard.
Those changes must be triggered from the Main-App with a special Tag:

SPLASH: {progress: 'XY', message: 'Message that will be displayed above the progressbar'}

Where XY should be the total amount of the progress already made during the loading of the app.
This value is handeled to the progress bar to display the current status.
The app closes itself after 100 has been reached.

You can use your own splash-image:
- simply put a file called 'splash.jpg' in the same folder as the splash-Screen-exe.
- If non is found, it falls back to the default-screen.


Current project state is really basic and just supported for
- Windows XP + .Net 4.0
- Windows Vista/7/8/8.1

just Windows XP needs .Net 4.0
If someone can tell me why, you're welcome :-)

The project-files are used with Visual Studio 2012, but the compilation is configured for Visual C++ 10.
