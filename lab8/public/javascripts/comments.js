/**
 * Comments (Lab8) front end UI JS
 */
jQuery(document).ready(function($) {
    // Configuration
    var comentAPIurl = 'comment';

    // Event handling
    $('#commentForm').on('submit', function(event) {
        // Prevent the form from submitting
        event.preventDefault();

        var nameElm =  $("#Name"),
            commentElm = $("#Comment");
        var name = nameElm.val();
        var comment = commentElm.val();
        if (name == '') {
            nameElm.parent().addClass('has-error');
        } else {
            nameElm.parent().removeClass('has-error');
        }
        if (comment == '') {
            commentElm.parent().addClass('has-error');
        } else {
            commentElm.parent().removeClass('has-error');
        }
        if (name == '' || comment == '') {
            $('#submitMsg').text('Please fill in all required fields.');
            return false;
        }

        // Serialize the comment
        var jsObj = JSON.stringify({
            Name: name,
            Comment: comment
        });
        $("#json").text(jsObj);

        // Post the comment
        $.ajax({
            url: comentAPIurl,
            type: 'POST',
            data: jsObj,
            contentType: 'application/json; charset=utf-8'
        }).then(
            function(data, textStatus) {
                $("#submitMsg").text(textStatus);
            },
            function(error) {
                console.warn('Problem posting comments: ', error);
                $('#submitMsg').text('Error');
            }
        );
    });

    $('#getCommentsBtn').click(function() {
        $.getJSON(comentAPIurl, function(data) {
            //console.log(data);
            var commentList = $('<ul/>');
            $.each(data, function(i, comment) {
                // Gravatar
                var imageId = comment.Name.trim().toLowerCase();

                commentList.append(
                    $('<li>')
                        .text('Name: ' + comment.Name + ' -- Comment: ' + comment.Comment)
                        .prepend(
                            $('<img/>').attr({
                                "src": '//www.gravatar.com/avatar/' + comment.gravatar + '?s=20&d=identicon',
                                "alt": '',
                                "class": 'img-thumbnail'
                            })
                        )
                );
            });
            $("#comments").html(commentList);
        });
    });
});