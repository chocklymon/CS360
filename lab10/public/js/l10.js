/**
 * Created by curtis on 3/28/16.
 */
angular.module('clusterApp', [])
    .controller('MainCtrl', [
        '$scope',
        '$http',
        function($scope, $http) {
            var completed = 0,
                timesToQuery = 100,
                pids = [];
            $scope.cluster = [];
            $scope.busy = false;

            $scope.getMyPIDs = function() {
                $scope.busy = true;
                for (var i = 0; i < timesToQuery; i++) {
                    $http.get('/pid').success(function(data) {
                        //console.log("getAll", data);
                        pids.push(data);
                        completed++;
                        if (completed % timesToQuery == 0) {
                            $scope.cluster = angular.copy(pids);
                            $scope.busy = false;
                        }
                    });
                }
            };
            $scope.clearPIDs = function() {
                completed = 0;
                pids = [];
                $scope.cluster = [];
            };
        }
    ]);