var express = require('express');
var router = express.Router();

/* GET home page. */
router.get('/', function(req, res, next) {
    res.render('index', { title: 'Express' });
});

// Comments
var crypto = require('crypto');

/* Set up mongoose in order to connect to mongo database */
var mongoose = require('mongoose');

mongoose.connect('mongodb://localhost/commentDB'); //Connects to a mongo database called "commentDB"

var commentSchema = mongoose.Schema({ // Defines the Schema for this database
    Name: String,
    Comment: String
});

var Comment = mongoose.model('Comment', commentSchema); // Makes an object from that schema as a model

var db = mongoose.connection; // Saves the connection as a variable to use
db.on('error', console.error.bind(console, 'connection error:')); // Checks for connection errors
db.once('open', function() { // Lets us know when we're connected
    console.log('Connected');
});

// Add comments to the database
router.post('/comment', function(req, res) {
    //console.log("POST comment route");

    var newcomment = new Comment(req.body);
    //console.log(newcomment);
    newcomment.save(function(err, post) {
        if (err) {
            return console.error(err);
        }
        //console.log(post);
        res.sendStatus(200);
    });
});

// GET comments from database
router.get('/comment', function(req, res) {
    //console.log("In the GET route");

    Comment.find(function(err, results) { //Calls the find() method on your database
        if (err) {
            // Print out the error and exit
            return console.error(err);
        }
        // Send the found comments
        //console.log(results);

        // Generate the gravatar image ids
        var comments = [], hasher, hash;
        for (var i = 0, l = results.length; i < l; i++) {
            hasher = crypto.createHash('md5');
            hasher.update(results[i].Name.trim().toLowerCase());
            hash = hasher.digest('hex');

            comments.push({
                Name: results[i].Name,
                Comment: results[i].Comment,
                gravatar: hash
            });
        }

        res.json(comments);
    });
});


module.exports = router;
