function startScanning() {
    $('#prediction-img').html("");
    $('#progress-wrapper').fadeIn(250);

    $.ajax({
        type: 'GET',
        url: "/start-predictions",
        success: function(response) {
            $('#progress-wrapper').fadeOut(250);

            if (response["status"] == "OK") {
                $('#btn-start-scanning').text("scanning...")
                $("#btn-start-scanning").prop("disabled", true);
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

        if (response["status"] == "OK") {
            if (response["device_img"] == "/") {
                htmlData += '<h4 class="text-center">Predicted device</h4>';
                htmlData += '<div id="prediction-img">';
                htmlData += '<p class="empty-prediction text-center mt-4">No device prediction could be found</p>';
            }

            else {
                htmlData += '<h4 class="text-center">Predicted device<br>' + response["device_type"] + '</h4>';
                htmlData += '<div id="prediction-img">';
                htmlData += response["device_img"];
            }
        } else {
            htmlData += '<h4 class="text-center">Predicted device</h4>';
            htmlData += '<div id="prediction-img">';
            htmlData += '<p class="error-prediction text-center mt-4">Failed to predict device</p>';
        }

        htmlData += '</div>';

        $('#prediction-data').html(htmlData);
        $('#prediction-data').fadeIn(250);

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