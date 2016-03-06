var express = require('express');
var router = express.Router();

// Get City Endpoint
router.get('/getcity', function(req, res) {
    var fs = require('fs');
    fs.readFile(__dirname + '/cities.dat.txt', function(err, data) {
        if (err) {
            throw err;
        }
        var myRe = new RegExp("^" + req.query.q);
        var cities = data.toString().split("\n");
        var result, foundCities = [];
        for (var i = 0; i < cities.length; i++) {
            result = cities[i].search(myRe);
            if (result != -1) {
                foundCities.push({city: cities[i]});
            }
        }
        res.status(200).jsonp(foundCities);
    });
});

// The random sentence generator
router.get('/randomsentence', function(req, res) {
    var fs = require('fs');

    function getRandom(args) {
        return args[Math.floor(Math.random() * args.length)];
    }

    fs.readFile(__dirname + '/sentences.json', function(err, data) {
        if (err) {
            throw err;
        }
        var sentences = JSON.parse(data.toString());
        var sentence = getRandom(sentences.subjects) + ' ' + getRandom(sentences.verbs)
            + ' ' + getRandom(sentences.objects) + getRandom(sentences.endings);

        res.status(200).jsonp(sentence);
    });
});


module.exports = router;
