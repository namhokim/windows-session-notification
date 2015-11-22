var fs = require('fs');
var requireUncached = require('require-uncached');

const pollingPeriodMilliSeconds = 15007;

var FileConfig = function (filename, callbackChangeNotification) {
    if (typeof filename !== 'string') {
        return;
    }

    const requireFilename = './' + filename;
    const trackFilename = requireFilename + '.json';
    var hasCallback = (typeof callbackChangeNotification === 'function');

    if (hasCallback) {
        var option = { persistent: true, interval: pollingPeriodMilliSeconds };
        fs.watchFile(trackFilename, option, function (curr, prev) {
            if (curr.mtime !== prev.mtime) {
                if (hasCallback) {
                    // read config file again
                    var newConfig = requireUncached(requireFilename);
                    callbackChangeNotification(newConfig);
                }
            }
        });
    }

    return require(requireFilename);
};

module.exports = FileConfig;