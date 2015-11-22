var KeyValueMap = function() {
    this.map = [];
};

KeyValueMap.prototype.get = function (key) {
    if (typeof this.map !== 'object') {
        return key;
    }
    var candidatedValue = this.map[key];
    if (typeof candidatedValue === 'undefined') {
        return key;
    }
    return candidatedValue;
};

KeyValueMap.prototype.set = function (mapData) {
    this.map = mapData;
};

module.exports = KeyValueMap;
