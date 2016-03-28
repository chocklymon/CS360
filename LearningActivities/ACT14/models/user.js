/**
 * Created by curtis on 3/25/16.
 */
var mongoose = require('mongoose');

module.exports = mongoose.model('User', {
    id: String,
    username: String,
    password: String,
    email: String,
    firstName: String,
    lastName: String
});