(function(scope) {
    var gui = require('nw.gui'),
        clipboard = gui.Clipboard.get(),
        totalProgress = 0,
        currentMessage = '',
        maximizeOnce = false;
    var win = gui.Window.get();
    /**
     Exports
     */
    scope.splashInfo = {};
    scope.splashInfo.maximize = true;
    scope.splashInfo.logging = false;

    scope.splashInfo.set = function(info, progress, to) {
        if (totalProgress == undefined) totalProgress = 0;
        var setInfoInvoker = function() {
            doSetInfo(info, progress)
        };
        if ( to && to > 0 ) {
            setTimeout(setInfoInvoker, to);
        } else {
            setInfoInvoker(info, progress);
        }
    };
    // \

    var doSetInfo = function(info, progress) {
        currentMessage = info;
        totalProgress += progress;
        totalProgress = Math.min(totalProgress, 100);
        scope.splashInfo.logging && console.log("SPLASH: {progress: '" + totalProgress + "', message:'" + currentMessage + "'}");
        clipboard.set("SPLASH: {progress: '" + totalProgress + "', message:'" + currentMessage + "'}", 'text');

        if ( totalProgress >= 100 && !maximizeOnce ) {
            maximizeOnce = true;
            if ( scope.splashInfo.maximize ) win.maximize();
            win.show();
        }
    }
})(window);
