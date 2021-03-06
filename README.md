#Splashscreen-Clipboard

A splash screen application. Currently just for for node-webkit (or any other app called nw.exe.)
Calls the Main-App (nw.exe) in a Child-Process and waits for changes in the Main-App.
The Communication-channel is the clipboard.

##How to use
You can use the exe in the Release-Folder or compile your own. 

Then just add the JS/splashinfo.js to your app and call
```js
splashInfo.set('Message you want to show above the progress bar', 10, [waitTime/Timeout]);
```

splashInfo.set(message, progress, [waitTime/Timeout]);

message: this is displayed above the progress bar

progress: the amount of progress that should be added to the total progress.

waitTime: an optional parameter where you can specify a timeout, e.g. for testing purposes.

You can use your own splash-image:
- simply put a file called *splash.png* in the same folder as the splash-Screen-exe.
- If non is found, it falls back to the default-screen, which is plain white, but still displays the progress bar.


##Specs
The Clipboard input has to look like the following:

SPLASH: {progress: 'XY', message: 'Message that will be displayed above the progressbar'}

Where XY should be the total amount of the progress already made during the loading of the app.
This value is handeled to the progress bar to display the current status.
The app closes itself after 100 has been reached.

Current project state is really basic and just supported for
- Windows XP/Vista/7/8/8.1

There was a bug, that Windows XP needed .Net 4.0, but seems to be fixed now.
There was a dependency with mscvr110.dll. But the compiler includes the dll now into the exe.

The project-files are used with Visual Studio 2012, but the compilation is configured for Visual C++ 10.