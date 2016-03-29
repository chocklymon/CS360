var express = require('express');
var router = express.Router();

/* GET home page. */
router.get('/', function(req, res, next) {
    res.render('index', { title: 'Express', pid: process.pid });
});

router.get('/pid', function(req, res, next) {
    console.log("Service ID " + process.pid);
    res.json({title: 'Cluster', pid: process.pid});
});

module.exports = router;
