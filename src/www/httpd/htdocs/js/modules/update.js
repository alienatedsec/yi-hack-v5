var APP = APP || {};

APP.update = (function ($) {

    let configs={};
    
    var uploadSection;
    var uploadButton;
    
    var fwpkgUploadButton;
    var fwpkgInstallButton;
    var fwpkgStatus;
    var fwpkgUploaded=false;

    var filesToUpload;

    function init() {
        registerEventHandler();
        fetchConfigs();
        
        uploadButton=$("#button-upload");
        
        uploadSection=$("#upload-section");
        //uploadSection.hide();

        fwpkgUploadButton=$("#button-fwpkg-upload");
        fwpkgInstallButton=$("#button-fwpkg-install");
        fwpkgStatus=$("#fwpkg-status");
    }

    function registerEventHandler() {
        $(document).on("click", '#button-upload', function (e) {
            uploadFiles();
        });
        $(document).on("click", '#button-fwpkg-upload', function (e) {
            uploadFwPackage();
        });
        $(document).on("click", '#button-fwpkg-install', function (e) {
            installFwPackage();
        });
    }

    function fetchConfigs() {
        loadingStatusElem = $('#loading-status');
        loadingStatusElem.text("Loading...");
       
        $.ajax({
            type: "GET",
            url: 'cgi-bin/update_backend.sh',
            dataType: "json",
            success: function(response) {
                loadingStatusElem.fadeOut(500);
                $.each(response, function (key, state) {
                    configs[key]=state;
                    
                    if($('input[type="checkbox"][data-key="' + key +'"]').length)
                        $('input[type="checkbox"][data-key="' + key +'"]').prop('checked', state === 'yes');
                    else if($('input[type="text"][data-key="' + key +'"]').length)
                        $('input[type="text"][data-key="' + key +'"]').prop('value', state);
                    
                });
                
                updateChecks();
            },
            error: function(response) {
                console.log('error', response);
            }
        });
    }
    
    function uploadFiles()
    {
        if(configs["IS_SD_PRESENT"]=="NO")
        {
            alert("An SD card must be present in the camera to flash a new image.");
            return;
        }

        var homeFilename=getFilename($("#file-home").val());
        var rootfsFilename=getFilename($("#file-rootfs").val());
        
        if(homeFilename=="" || rootfsFilename=="")
        {
            alert("Both home and rootfs files must be supplied.");
            return;
        }
        
        if(!homeFilename.startsWith("home"))
        {
            alert("Wrong home filename.");
            return;
        }
        
        if(!rootfsFilename.startsWith("rootfs"))
        {
            alert("Wrong rootfs filename.");
            return;
        }
        
        buttonShowUploading();
        
        filesToUpload=0;
        
        uploadFirmwareImage("#home-form", "#file-home", homeFilename, function(){ 
            console.log("Home uploaded!"); 
            uploadFinished();
         });
         
        uploadFirmwareImage("#rootfs-form", "#file-rootfs", rootfsFilename, function(){
            console.log("Rootfs uploaded!");
            uploadFinished();
        });
    }
    
    function uploadFirmwareImage(formId, fileId, filename, callback)
    {
        filesToUpload++;
        console.log("Uploading " + filename);
        var fd = new FormData();
        fd.append('file',$(fileId)[0].files[0]);
        ajaxUpload(fd, filename, callback);
    }
    
    function uploadFinished()
    {
        filesToUpload--;
        if(filesToUpload==0)
        {
            buttonShowDone();
            alert("Update files successfully uploaded!\n\nNow reboot your camera and wait a couple of minutes for the process to complete.");
        }
    }
    
    function ajaxUpload(fd, fileName, callback)
    {
        $.ajax({
            url: "cgi-bin/upload.sh?file=" + fileName,  
            type: 'POST',
            data: fd,
            success: function(data){
                console.log(data);
                callback(data);
            },
            cache: false,
            contentType: false,
            processData: false
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
            success: function(data){
                fwpkgUploadButton.prop('value', 'Upload');
                fwpkgUploaded=true;
                fwpkgInstallButton.prop('disabled', false);
                updateFwPkgStatus("Package uploaded. Ready to install.");
            },
            error: function(){
                fwpkgUploadButton.prop('value', 'Upload');
                updateFwPkgStatus("Upload failed.");
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
    
    function updateChecks()
    {
        var verStatusElem=$("#version-status");
        var needsUpdate=configs["NEEDS_UPDATE"];
        var isSdPresent=configs["IS_SD_PRESENT"]=="YES" ? true : false;

        if(!isSdPresent)
        {
            var elem=$("#upload-tbody");
            elem.text("An SD card must be present in the camera to flash a new image.")
            var fwpkgElem=$("#fwpkg-tbody");
            fwpkgElem.text("An SD card must be present in the camera to install a firmware package.")
            fwpkgInstallButton.prop('disabled', true);
        }
        
        if(needsUpdate=="yes")
        {
            uploadSection.show();
            verStatusElem.text("A new version is available!");
        }
        else if(needsUpdate=="no")
            verStatusElem.text("You are up to date!");
        else if(needsUpdate=="no_currentversionisbeta")
            verStatusElem.text("Congratulations, you are using a pre-release version!");
    }
    
    function buttonShowUploading()
    {
        uploadButton.prop('value', 'Uploading...');
    }
    
    function buttonShowDone()
    {
        uploadButton.prop('value', 'Done!');
    }
    
    function getFilename(path)
    {
        return path.split(/(\\|\/)/g).pop();
    }
    
    return {
        init: init
    };

})(jQuery);
