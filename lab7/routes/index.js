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
        res.status(200).json(foundCities);
    });
});

/* GET home page. */
//router.get('/', function(req, res, next) {
//    res.render('index', { title: 'Express' });
//});

module.exports = router;
