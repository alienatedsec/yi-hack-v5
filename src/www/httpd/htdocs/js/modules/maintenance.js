var APP = APP || {};

APP.maintenance = (function($) {

    var timeoutVar;

    var fwpkgUploadButton;
    var fwpkgInstallButton;
    var fwpkgStatus;
    var fwpkgUploaded=false;
    var configs={};

    function init() {
        registerEventHandler();
        setRebootStatus("Camera is online.");
        getFwStatus();
        getPreFwStatus();
        initFwPackage();
    }

    function registerEventHandler() {
        $(document).on("click", '#button-save', function(e) {
            saveConfig();
        });
        $(document).on("click", '#button-load', function(e) {
            loadConfig();
        });
        $(document).on("click", '#button-reboot', function(e) {
            rebootCamera();
        });
        $(document).on("click", '#button-reset', function(e) {
            resetCamera();
        });
        $(document).on("click", '#button-upgrade', function(e) {
            upgradeFirmware();
        });
        $(document).on("click", '#button-preupgrade', function(e) {
            preupgradeFirmware();
        });
        $(document).on("click", '#button-fwpkg-upload', function(e) {
            uploadFwPackage();
        });
        $(document).on("click", '#button-fwpkg-install', function(e) {
            installFwPackage();
        });
    }

    function saveConfig() {
        $('#button-save').attr("disabled", true);
        var xhr = new XMLHttpRequest();
        xhr.open('GET', 'cgi-bin/save.sh', true);
        xhr.responseType = 'blob';
        xhr.onload = function(e) {
            if (xhr.status == 200) {
                var myBlob = xhr.response;
                var url = URL.createObjectURL(myBlob);
                var $a = $('<a />', {
                    'href': url,
                    'download': 'config.tar.bz2',
                    'text': "click"
                }).hide().appendTo("body")[0].click();
                URL.revokeObjectURL(url);
            }
        };
        xhr.send();
    }

    function loadConfig() {
        $('#button-load').attr("disabled", true);
        var fileSelect = document.getElementById('button-file');
        var files = fileSelect.files;
        var formData = new FormData();

        for (var i = 0; i < files.length; i++) {
            var file = files[i];
            formData.append('files[]', file, file.name);
        }

        var xhr = new XMLHttpRequest();
        xhr.open('POST', 'cgi-bin/load.sh', true);
        xhr.onload = function() {
            if (xhr.status === 200) {
                $('#button-load').attr("disabled", false);
            }
            var myText = xhr.response;
            $('#text-load').text(myText);
        };
        xhr.send(formData);
    }

    function rebootCamera() {
        $('#button-reboot').attr("disabled", true);
        var x=confirm("Are you sure you want to reboot?");
        if (x) {
            $.ajax({
                type: "GET",
                url: 'cgi-bin/reboot.sh',
                dataType: "json",
                error: function(response) {
                    console.log('error', response);
                    $('#button-reboot').attr("disabled", false);
                },
                success: function(data) {
                    setRebootStatus("Camera is rebooting.");
                    waitForBoot();
                }
            });
        }
    }

    function waitForBoot() {
        setInterval(function() {
            $.ajax({
                url: 'index.html',
                cache: false,
                success: function(data) {
                    setRebootStatus("Camera is back online, redirecting to home.");
                    $('#button-reboot').attr("disabled", false);
                    window.location.href = "index.html";
                },
                error: function(data) {
                    setRebootStatus("Waiting for the camera to come back online.");
                },
                timeout: 3000,
            });
        }, 5000);
    }

    function resetCamera() {
        $('#button-reset').attr("disabled", true);
        var x=confirm("Are you sure you want to reset?");
        if (x) {
            $.ajax({
                type: "GET",
                url: 'cgi-bin/reset.sh',
                dataType: "json",
                error: function(response) {
                    console.log('error', response);
                    $('#button-reset').attr("disabled", false);
                },
                success: function(data) {
                    setResetStatus("Reset completed, reboot your camera.");
                }
            });
        }
    }

    function upgradeFirmware() {
        $('#button-upgrade').attr("disabled", true);
        setFwStatus("Firmware download in progress.");
        $.ajax({
            type: "GET",
            url: 'cgi-bin/fw_upgrade.sh?get=upgrade',
            error: function(response) {
                console.log('error', response);
                $('#button-upgrade').attr("disabled", false);
            },
            success: function(response) {
                setFwStatus(response);
                waitForUpgrade("button-upgrade");
            }
        });
    }
    
    function preupgradeFirmware() {
        $('#button-preupgrade').attr("disabled", true);
        setPreFwStatus("Pre-Release Firmware download in progress.");
        $.ajax({
            type: "GET",
            url: 'cgi-bin/fw_upgrade.sh?get=preupgrade',
            error: function(response) {
                console.log('error', response);
                $('#button-preupgrade').attr("disabled", false);
            },
            success: function(response) {
                setPreFwStatus(response);
                waitForUpgrade("button-preupgrade");
            }
        });
    }
    
    function waitForUpgrade(buttonType) {
        setInterval(function() {
            $.ajax({
                url: 'index.html',
                cache: false,
                success: function(data) {
                    if (buttonType === "button-upgrade") {
                        setFwStatus("Camera is upgrading.");
                    } else if (buttonType === "button-preupgrade") {
                        setPreFwStatus("Pre-upgrade in progress...");
                    }

                    $('#' + buttonType).attr("disabled", false);
                    window.location.href = "index.html";
                },
                error: function(data) {
                    setFwStatus("Waiting for the camera to come back online...");
                    setPreFwStatus("Waiting for the camera to come back online...");
                },
                timeout: 3000,
            });
        }, 5000);
    }

    
    function setRebootStatus(text) {
        $('input[type="text"][data-key="STATUS"]').prop('value', text);
    }

    function setResetStatus(text) {
        $('input[type="text"][data-key="RESET"]').prop('value', text);
    }

    function setFwStatus(text) {
        $('input[type="text"][data-key="FW"]').prop('value', text);
    }
    
    function setPreFwStatus(text) {
        $('input[type="text"][data-key="PREFW"]').prop('value', text);
    }

    function getFwStatus() {
        $.ajax({
            type: "GET",
            url: 'cgi-bin/fw_upgrade.sh?get=info',
            dataType: "json",
            error: function(response) {
                console.log('error', response);
                setFwStatus("Error getting fw info");
            },
            success: function(data) {
                if (data.checking) {
                    // The camera is checking GitHub in the background -
                    // re-poll until the check lands in the cache.
                    setFwStatus("Installed: " + data.fw_version + " - checking for updates...");
                    setTimeout(getFwStatus, 5000);
                    return;
                }
                setFwStatus("Installed: " + data.fw_version + " - Available: " + data.latest_fw);
                if (data.fw_version == data.latest_fw) {
                    $('#button-upgrade').attr("disabled", true);
                }
            }
        });
    }
    
    function getPreFwStatus() {
        $.ajax({
            type: "GET",
            url: 'cgi-bin/fw_upgrade.sh?get=info',
            dataType: "json",
            error: function(response) {
                console.log('error', response);
                setPreFwStatus("Error getting fw info");
            },
            success: function(data) {
                if (data.checking) {
                    setPreFwStatus("Installed: " + data.fw_version + " - checking for pre-releases...");
                    setTimeout(getPreFwStatus, 5000);
                    return;
                }
                setPreFwStatus("Installed: " + data.fw_version + " - Available: " + data.prerelease_fw);
                if (data.fw_version == data.prerelease_fw) {
                    $('#button-preupgrade').attr("disabled", true);
                }
            }
        });
    }
    
    function initFwPackage() {
        fwpkgUploadButton=$("#button-fwpkg-upload");
        fwpkgInstallButton=$("#button-fwpkg-install");
        fwpkgStatus=$("#fwpkg-status");
        $.ajax({
            type: "GET",
            url: 'cgi-bin/update_backend.sh',
            dataType: "json",
            success: function(response) {
                $.each(response, function (key, state) {
                    configs[key]=state;
                });
                if(response["IS_SD_PRESENT"]=="NO") {
                    $("#fwpkg-tbody").text("An SD card must be present in the camera to install a firmware package.");
                    fwpkgInstallButton.prop('disabled', true);
                }
            },
            error: function(response) {
                console.log('error', response);
            }
        });
    }

    function updateFwPkgStatus(text)
    {
        if(fwpkgStatus)
            fwpkgStatus.text(text);
    }

    function uploadFwPackage()
    {
        if(configs["IS_SD_PRESENT"]=="NO")
        {
            alert("An SD card must be present in the camera to install a firmware package.");
            return;
        }

        var fileInput=document.getElementById("file-fwpkg");
        if(!fileInput || !fileInput.files || fileInput.files.length==0)
        {
            alert("Choose a .tgz package file first.");
            return;
        }

        var fname=fileInput.files[0].name;
        if(!fname.toLowerCase().endsWith(".tgz"))
        {
            alert("Only .tgz packages are supported.");
            return;
        }

        fwpkgUploadButton.prop('value', 'Uploading...');
        updateFwPkgStatus("Uploading " + fname + " ... this can take a while.");

        var fd = new FormData();
        fd.append('file', fileInput.files[0]);
        $.ajax({
            url: "cgi-bin/upload.sh?file=fw_upload.tgz",
            type: 'POST',
            data: fd,
            cache: false,
            contentType: false,
            processData: false,
            // The camera's Wi-Fi link is slow (~1 MB in 7 s), so show
            // the transfer percentage - a frozen "Uploading..." makes
            // users think the upload is dead and abort it.
            xhr: function() {
                var x = new XMLHttpRequest();
                x.upload.addEventListener("progress", function(e) {
                    if (e.lengthComputable) {
                        updateFwPkgStatus("Uploading " + fname + " ... " + Math.round(e.loaded / e.total * 100) + "%");
                    }
                });
                return x;
            },
            success: function(data){
                fwpkgUploadButton.prop('value', 'Upload');
                if (data && String(data.error) === "true") {
                    // The CGI verified the transfer and found it short.
                    updateFwPkgStatus((data.message || "Upload failed.") + " Please retry.");
                    return;
                }
                fwpkgUploaded=true;
                fwpkgInstallButton.prop('disabled', false);
                updateFwPkgStatus("Package uploaded. Ready to install.");
            },
            error: function(xhr){
                fwpkgUploadButton.prop('value', 'Upload');
                if (xhr && xhr.status === 0) {
                    // Network-level failure: the connection dropped
                    // before the transfer finished.
                    updateFwPkgStatus("Upload failed - the connection dropped before the transfer finished. Please retry.");
                } else {
                    updateFwPkgStatus("Upload failed (HTTP " + (xhr ? xhr.status : "unknown") + "). Please retry.");
                }
            }
        });
    }

    function installFwPackage()
    {
        if(!fwpkgUploaded)
        {
            alert("Upload a package first.");
            return;
        }

        if(!confirm("Install the uploaded package?\nThe camera will reboot and take a couple of minutes."))
            return;

        fwpkgInstallButton.prop('value', 'Installing...');
        updateFwPkgStatus("Installing. The camera will reboot in a few seconds.");

        $.ajax({
            url: "cgi-bin/fw_upgrade.sh?get=tgz",
            type: "GET",
            timeout: 30000,
            success: function(data){
                if(String(data).indexOf("Download completed") >= 0)
                    updateFwPkgStatus(data + " The camera will reboot shortly.");
                else
                    updateFwPkgStatus(data);
            },
            error: function(){
                updateFwPkgStatus("Install started. The camera will reboot shortly.");
            }
        });
    }

    return {
        init: init
    };

})(jQuery);
