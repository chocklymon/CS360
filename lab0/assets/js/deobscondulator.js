/*!
 * De-Obscondulator
 * Takes obscured data elements and displays them in browsers that run javascript.
 *
 * This is designed to hide email addresses and other data from bots that scrape web pages for this data. Most bots
 * don't run javascript.
 *
 * Basic Usage:
 * Add a 'data-obs' attribute to any element. The contents of the data attribute will be used to replace the element's
 * current contents.
 *
 * Data format: [data type]:[data]
 * [data type] string 'eml' for email addresses, 'text' for other data types.
 * [data] string. The data to be revaled. This is a base64 encoded string of the URI encoded data to display.
 *
 * @author Curtis Oakley
 */

// Make sure that there is a console
if (!('console' in window)) {
    console = {
        log: function () {}
    };
}

// The De-Obscondulator
jQuery(function ($) {
    $('*[data-obs]').each(function (i, el) {
        var $el = $(el);
        var data = $el.attr('data-obs').split(':'), html;
        if (data.length == 2) {
            var text = decodeURIComponent(atob(data[1]));
            if (data[0] == 'eml') {
                var splitLoc = text.indexOf('|');
                if (splitLoc == -1) {
                    console.log('Obscondulator error: Invalid format', $el);
                    return;
                }
                text = text.substring(splitLoc + 1) + '&#64;' + text.substring(0, splitLoc);
                html = '<a href="mailto:' + text + '">' + text + '</a>';
            } else {
                html = text;
            }

            // Replace contents with plain text
            $el.empty();
            $el.html(html);
        } else {
            console.log('Obscondulator error: Invalid data', el);
        }
    });
});
