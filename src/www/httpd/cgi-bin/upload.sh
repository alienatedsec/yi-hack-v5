#!/bin/sh

# 0.4.1
# 0.4.3d: verify against CONTENT_LENGTH that the multipart body arrived
# complete - a dropped connection used to leave a partial file that was
# silently reported as a successful upload.

YI_HACK_PREFIX="/tmp/sd/yi-hack-v5"

get_file_type()
{   
    CONF="$(echo $QUERY_STRING | cut -d'=' -f1)"
    VAL="$(echo $QUERY_STRING | cut -d'=' -f2)"
    
    if [ $CONF == "file" ] ; then
        echo $VAL
    fi
}

get_random_tmp_file()
{
    local CNT=5
    local RND=""
    local TMP_FILE=""
    
    while : ; do
        RND=$(</dev/urandom tr -dc 0-9 | dd bs=$CNT count=1 2>/dev/null | sed -e 's/^0\+//' )
        TMP_FILE="/tmp/.tmpupload.$RND"
        
        [ -f $TMP_FILE ] || break
    done
    
    echo $TMP_FILE
}

get_file_from_post()
{
    local FILE=$1

    local CR=`printf '\r'`

    IFS="$CR"
    read -r delim_line
    # Bytes consumed so far: the delimiter line plus its CRLF. The
    # delimiter is read with CR as the field separator, so it does not
    # include the CR itself.
    HDR_BYTES=$(( ${#delim_line} + 2 ))
    IFS=""

    while read -r line; do
        # CR is kept in the line here (empty IFS), only the LF is
        # stripped, so each line costs its length plus one byte.
        HDR_BYTES=$(( HDR_BYTES + ${#line} + 1 ))
        test x"$line" = x"" && break
        test x"$line" = x"$CR" && break
    done

    cat > "$FILE"

    # We need to delete the tail of "\r\ndelim_line--\r\n"
    tail_len=$((${#delim_line} + 6))

    # Get and check file size
    filesize=`ls -l "$FILE" | awk '{print $5}'`

    # Truncate the file (skip if the trailer is missing - a truncated
    # upload, handled by the caller)
    if [ "$filesize" -gt "$tail_len" ] ; then
        dd of="$FILE" seek=$((filesize - tail_len)) bs=1 count=0 >/dev/null 2>/dev/null
    fi
}

print_error()
{
    printf "{\n"
    printf "\"%s\":\"%s\",\n"  "error" "true"
    printf "\"%s\":\"%s\"\n"  "message" "$1"
    printf "}"
}

printf "Content-type: application/json\r\n\r\n"

FILE_TYPE="$(get_file_type)"
TMP_FILE=$(get_random_tmp_file)

CUT_FILE_TYPE=$(echo $FILE_TYPE | cut -d'_' -f1)

if [[ "$CUT_FILE_TYPE" == "home" || "$CUT_FILE_TYPE" == "rootfs" ]] ; then
    # If home or rootfs image, place directly on sd card
    get_file_from_post "/tmp/sd/$FILE_TYPE"
    RECV_FILE="/tmp/sd/$FILE_TYPE"
else
    get_file_from_post $TMP_FILE
    RECV_FILE=$TMP_FILE
fi

# The multipart body is exactly: consumed header lines + file data +
# trailer. httpd passes the exact body size in CONTENT_LENGTH, so a
# short file means the connection dropped mid-transfer (httpd then
# closes the CGI's stdin and the remaining bytes never arrive). Report
# it and remove the partial file instead of advertising a broken upload
# as a success.
if [ -n "$CONTENT_LENGTH" ] ; then
    EXPECTED_SIZE=$((CONTENT_LENGTH - HDR_BYTES - tail_len))
    RECEIVED_SIZE=$((filesize - tail_len))
    if [ "$RECEIVED_SIZE" -ne "$EXPECTED_SIZE" ] ; then
        rm -f "$RECV_FILE"
        print_error "Upload incomplete ($RECEIVED_SIZE of $EXPECTED_SIZE bytes received) - the connection dropped mid-transfer, please retry"
        exit 0
    fi
fi

if [ "$CUT_FILE_TYPE" != "home" ] && [ "$CUT_FILE_TYPE" != "rootfs" ] ; then
    cp -f "$TMP_FILE" "$YI_HACK_PREFIX/$FILE_TYPE"
fi

rm -f $TMP_FILE

printf "{\n"

printf "\"%s\":\"%s\"\n"  "error" "false"

printf "}"

exit 0
