function startScanning() {
    $('#prediction-img').html("");
    $('#progress-wrapper').fadeIn(250);

    $.ajax({
        type: 'GET',
        url: "/start-predictions",
        success: function(response) {
            $('#progress-wrapper').fadeOut(250);

            if (response["status"] == "OK") {
                showAlert("success", "Successfully started scanner for predicting device types");

                setTimeout(function() {
                    updatePrediction();
                }, 5000);
            } else
                showAlert("success", "Failed to start scanner for predicting device types");
        },
        error: function(error) {
            $('#progress-wrapper').fadeOut(250);
            showAlert("success", "Error occured while starting scanner for predicting device types");
        }
    });
}

function updatePrediction() {
    $.getJSON("/get-predictions", function(response) {
        var htmlData = [];
        htmlData += '<h3 class="text-center">Prediction</h3>';
        htmlData += '<div id="prediction-img">';

        if (response["status"] == "OK") {
            if (response["data"] == "/")
                htmlData += '<p class="empty-prediction text-center mt-5">No prediction could be found for any device</p>';
            else
                htmlData += response["data"];
        } else
            htmlData += '<p class="error-prediction text-center mt-5">Failed to predict device that is running or not</p>';

        htmlData += '</div>';

        $('#prediction-data').html(htmlData);

        setTimeout(function() {
            updatePrediction();
        }, 3500);
    });
}

function showAlert(type, message) {
    if (type == "success") {
        $('#alert-success').text(message);
        $('#alert-success').fadeIn(250).delay(3500).fadeOut(250);
    }

    if (type == "danger") {
        $('#alert-danger').text(message);
        $('#alert-danger').fadeIn(250).delay(3500).fadeOut(250);
    }
}