var APP = APP || {};

APP.ptz = (function ($) {

    function init() {
        updatePage();
        registerEventHandler();
        initPage();
        snapshotTimer();
    }

    function snapshotTimer() {
        interval = 4000;
        lastUpdate = Date.now();
        function updateImage() {
            jQuery.get('cgi-bin/snapshot.sh?res=low&base64=yes', function(data) {
                image = document.getElementById('imgSnap');
                image.src = 'data:image/png;base64,' + data;
                this_interval = Math.max(interval - (Date.now() - lastUpdate), 0);
             lastUpdate = Date.now();
            setTimeout(updateImage, this_interval);
            })
        }
        setTimeout(updateImage);
    }

    function registerEventHandler() {
        $(document).on("click", '#img-au', function (e) {
            move('#img-au', 'up');
        });
        $(document).on("click", '#img-al', function (e) {
            move('#img-al', 'left');
        });
        $(document).on("click", '#img-ar', function (e) {
            move('#img-ar', 'right');
        });
        $(document).on("click", '#img-ad', function (e) {
            move('#img-ad', 'down');
        });
        $(document).on("click", '#button-goto', function (e) {
            gotoPreset('#button-goto', '#select-goto');
        });
    }

    function move(button, dir) {
        $(button).attr("disabled", true);
        $.ajax({
            type: "GET",
            url: 'cgi-bin/ptz.sh?dir='+dir,
            dataType: "json",
            error: function(response) {
                console.log('error', response);
                $(button).attr("disabled", false);
            },
            success: function(data) {
                $(button).attr("disabled", false);
            }
        });
    }

    function gotoPreset(button, select) {
        $(button).attr("disabled", true);
        $.ajax({
            type: "GET",
            url: 'cgi-bin/preset.sh?action=go_preset&num='+$(select + " option:selected").text(),
            dataType: "json",
            error: function(response) {
                console.log('error', response);
                $(button).attr("disabled", false);
            },
            success: function(data) {
                $(button).attr("disabled", false);
            }
        });
    }

    function addPreset(button) {
        $(button).attr("disabled", true);
        $.ajax({
            type: "POST",
            url: 'cgi-bin/preset.sh?action=add_preset&name='+$('input[type="text"][data-key="PRESET_NAME"]').prop('value'),
            dataType: "json",
            error: function(response) {
                console.log('error', response);
                $(button).attr("disabled", false);
            },
            success: function(data) {
                $(button).attr("disabled", false);
                window.location.reload();
            }
        });
    }

    function delPreset(button, select) {
        $(button).attr("disabled", true);
        $.ajax({
            type: "POST",
            url: 'cgi-bin/preset.sh?action=del_preset&num='+$(select + " option:selected").val(),
            dataType: "json",
            error: function(response) {
                console.log('error', response);
                $(button).attr("disabled", false);
            },
            success: function(data) {
                $(button).attr("disabled", false);
                window.location.reload();
            }
        });
    }

    function delAllPreset(button) {
        $(button).attr("disabled", true);
        $.ajax({
            type: "POST",
            url: 'cgi-bin/preset.sh?action=del_preset&num=all',
            dataType: "json",
            error: function(response) {
                console.log('error', response);
                $(button).attr("disabled", false);
            },
            success: function(data) {
                $(button).attr("disabled", false);
                window.location.reload();
            }
        });
    }

    function initPage() {
        $.ajax({
            type: "GET",
            url: 'cgi-bin/get_configs.sh?conf=ptz_presets',
            dataType: "json",
            success: function(data) {
                html = "<select id=\"select-goto\">\n";
                for (let key in data) {
                    if (key != "NULL") {
                        fields = data[key].split(',');
                        html += "<option value=\"" + key + "\">" + key + " - " + fields[0] + "</option>\n";
                    }
                }
                html += "</select>\n";
                document.getElementById("select-goto-container").innerHTML = html;

                html = "<select id=\"select-del\">\n";
                for (let key in data) {
                    if (key != "NULL") {
                        fields = data[key].split(',');
                        html += "<option value=\"" + key + "\">" + key + " - " + fields[0] + "</option>\n";
                    }
                }
                html += "</select>\n";
                document.getElementById("select-del-container").innerHTML = html;
            },
            error: function(response) {
                console.log('error', response);
            }
        });
    }

    function updatePage() {
        $.ajax({
            type: "GET",
            url: 'cgi-bin/status.json',
            dataType: "json",
            success: function(data) {
                for (let key in data) {
                    if (key == "model_suffix") {
                        if ((data[key] == "yi_dome") || (data[key] == "yi_dome_1080p") || (data[key] == "yi_cloud_dome_1080p")) {
                            $('#ptz_description').show();
                            $('#ptz_available').hide();
                            $('#ptz_main').show();
                        } else {
                            $('#ptz_description').hide();
                            $('#ptz_available').show();
                            $('#ptz_main').hide();
                        }
                    }
                }
            },
            error: function(response) {
                console.log('error', response);
            }
        });
    }

    return {
        init: init
    };

})(jQuery);
