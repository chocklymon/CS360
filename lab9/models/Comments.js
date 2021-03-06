/**
 * Created by curtis on 3/16/16.
 */
var mongoose = require('mongoose');
var CommentSchema = new mongoose.Schema({
    title: String,
    name: String,
    gravatarId: String,
    upvotes: {type: Number, default: 0}
});

CommentSchema.methods.upvote = function(cb) {
    this.upvotes += 1;
    this.save(cb);
};

mongoose.model('Comment', CommentSchema);
