/**
 * Code for the lab6 project.
 * Loads the current weather for a Utah city.
 */
jQuery(function($) {
    // Variables
    var sentenceChangeTime = 10;// seconds
    var sentenceChangeCountdown = sentenceChangeTime;
    var sentenceChangeInterval;

    // Define functions //

    /**
     * Loads and displays the weather for the UT city currently in the city text input.
     */
    var loadWeather = function() {
        var city = $('#cityfield').val();
        $('#dispcity').val(city);

        $.ajax({
            method: 'GET',
            url: 'http://api.wunderground.com/api/f954553d21896a97/conditions/q/UT/' + city + '.json',
            dataType: 'json'
        }).then(
            function(data) {
                if (data && data.current_observation) {
                    var condition = data.current_observation.weather;

                    $('#weatherFor').text(
                        ' for ' + data.current_observation.display_location.city +
                        ', ' + data.current_observation.display_location.state
                    );
                    $('#cTemperatureF').text(data.current_observation.temp_f);
                    $('#cTemperatureC').text(data.current_observation.temp_c);
                    $('#cCondition').text(condition);

                    // Find the condition image
                    var conditionImage;
                    switch (data.current_observation.icon) {
                        case 'cloudy':
                            conditionImage = 'cloudy.svg';
                            break;
                        case 'mostlycloudy':// Fall through
                        case 'partlycloudy':
                        case 'partlysunny':
                            conditionImage = 'partly-cloudy.svg';
                            break;
                        case 'chancerain':
                            conditionImage = 'rain-storm.svg';
                            break;
                        case 'chanceflurries':// Fall through
                        case 'chancesleet':
                        case 'chancesnow':
                        case 'flurries':
                        case 'sleet':
                        case 'snow':
                            conditionImage = 'snow.svg';
                            break;
                        case 'clear':// Fall through
                        case 'mostlysunny':
                            conditionImage = 'sunny.svg';
                            break;
                        case 'chancetstorms':// Fall through
                        case 'tstorms':
                            conditionImage = 'thunderstorm.svg';
                            break;
                    }

                    // Show the appropriate condition image
                    if (conditionImage) {
                        $('#cConditionImg').attr({
                            src: 'images/weather/' + conditionImage,
                            title: condition,
                            alt: condition
                        }).show();
                    } else {
                        $('#cConditionImg').hide();
                    }

                    // Display the weather panel
                    $('#weatherPanel').show();
                    $('#emptyWeather').hide();
                }
            },
            function(error) {
                console.warn("AJAX Exception: Failed to get the current weather - ", error);
            }
        );
    };

    /**
     * Builds the suggestions list from the provided city data.
     * @param data Array of city items.
     */
    var buildSuggestions = function(data) {
        // Build the suggestions
        var suggestions = '';
        $.each(data, function(i, item) {
            suggestions += '<li><a href="#">' + item.city + '</a></li>';
        });
        $('#txtHint').html(suggestions);

        // Bind an event to the suggestions
        $('#txtHint a').on('click', function(e) {
            e.preventDefault();

            var city = $(e.target).text();
            $('#cityfield').val(city);

            loadWeather();
            buildSuggestions([{
                city: city
            }]);
        });
    };

    /**
     * Loads and displays a quote.
     */
    var getRandomSentence = function() {
        return $.ajax({
            method: 'GET',
            url: '/randomsentence',
            dataType: 'json'
        }).then(
            function (data) {
                var quoteBlock = $('#quote');
                quoteBlock.children('p').each(function(index) {
                    // Cycle older elements
                    if (index == 0) {
                        $(this).removeClass('sentence-first');
                    } else if (index == 1) {
                        $(this).addClass('text-muted');
                    } else if (index == 2) {
                        $(this).addClass('sentence-going');
                    } else {
                        $(this).remove();
                        return false;
                    }
                });
                quoteBlock.prepend('<p class="sentence-first">' + data + '</p>');
            },
            function (error) {
                console.warn("AJAX Exception: Failed to get a quote - ", error);
            }
        );
    };

    var autoGetSentence = function() {
        // Get the next sentence and reset if needed
        if (sentenceChangeCountdown == 0) {
            getRandomSentence();
            sentenceChangeCountdown = sentenceChangeTime;
        }

        // Display the countdown
        var countdownUnit = ' second';
        if (sentenceChangeCountdown > 1) {
            countdownUnit += 's';
        }
        $('#sentenceCountdown').text(sentenceChangeCountdown + countdownUnit);

        // Countdown
        sentenceChangeCountdown--;
    };

    // Bind Events //
    $('#cityfield').on('keyup', function() {
        var city = $('#cityfield').val();
        if (city.length > 0) {
            $.ajax({
                method: 'GET',
                url: '/getcity',
                data: {
                    q: city
                },
                dataType: 'json'
            }).then(
                buildSuggestions,
                function(error) {
                    console.warn("AJAX Exception: Failed to get the list of suggested cities - ", error);
                }
            );
        }
    });

    $('#weatherForm').on('submit', function(e) {
        e.preventDefault();
        loadWeather();
    });

    $('#getQuoteBtn').on('click', function() {
        $('#getQuoteBtn').prop('disabled', true);
        getRandomSentence().always(function() {
            sentenceChangeCountdown = sentenceChangeTime;
            $('#getQuoteBtn').prop('disabled', false);
        });
    });

    $('#autosentenceOn').on('change', function(event) {
        $(this).parent().addClass('active');
        $('#autosentenceOff').parent().removeClass('active');

        sentenceChangeInterval = setInterval(autoGetSentence, 1000);
    });

    $('#autosentenceOff').on('change', function(event) {
        $(this).parent().addClass('active');
        $('#autosentenceOn').parent().removeClass('active');

        clearInterval(sentenceChangeInterval);
    });

    // Set up the random sentence refresher
    sentenceChangeInterval = setInterval(autoGetSentence, 1000);

    // Load the initial quote
    getRandomSentence();
});
