/**
 * Created by curtis on 3/14/16.
 */
var l12 = angular.module('l12', []);

l12.controller('listController', ['$scope', function ($scope) {
    $scope.first = 'Some';
    $scope.last = 'One';
    $scope.heading = 'Message: ';
    $scope.updateMessage = function() {
        $scope.message = 'Hello ' + $scope.first.toUpperCase() +' '+ $scope.last.toUpperCase() + '!';
    };
}]);
