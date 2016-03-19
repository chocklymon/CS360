/**
 * AngularJS Module for Lab 9.
 */
var commentModule = angular.module('comment', []);
commentModule.controller('MainCtrl', ['$scope', '$http', '$log', function($scope, $http, $log) {
    // Comments Model
    $scope.comments = [];

    // Comment Functions
    var createComment = function(comment) {
        return $http.post('/comments', comment).success(function(data) {
            $scope.comments.push(data);
        });
    };

    var upvoteComment = function(comment) {
        return $http.put('/comments/' + comment._id + '/upvote').then(
            function(data) {
                $log.log("Upvote successful");
                comment.upvotes += 1;
            },
            function(error) {
                $log.warn("Upvote error: ", error);
            }
        );
    };

    // Load the current comments
    $http.get('/comments').success(function(data) {
        angular.copy(data, $scope.comments);
    });

    // Event functions
    $scope.addComment = function() {
        if ($scope.formContent) {
            var comment = {
                title: $scope.formContent,
                upvotes: 0
            };
            createComment(comment);
            $scope.formContent = '';
        }
    };
    $scope.incrementUpvotes = upvoteComment;
}]);
