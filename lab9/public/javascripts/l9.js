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

    // Form validation
    var attemptedAdd = false;
    $scope.formInvalid = function() {
        return $scope.addCommentForm.$invalid && $scope.addCommentForm.$dirty && attemptedAdd;
    };

    // Load the current comments
    $http.get('/comments').success(function(data) {
        angular.copy(data, $scope.comments);
    });

    // Event functions
    $scope.addComment = function() {
        attemptedAdd = true;
        if ($scope.addCommentForm.$valid) {
            var comment = {
                title: $scope.commentText,
                name: $scope.name,
                gravatarId: CryptoJS.MD5($scope.email.trim().toLowerCase()).toString(),
                upvotes: 0
            };
            createComment(comment).finally(function() {
                attemptedAdd = false;
            });
            $scope.commentText = '';
            $scope.name = '';
            $scope.email = '';
        }
    };
    $scope.incrementUpvotes = upvoteComment;
}]);
