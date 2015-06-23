"use strict";

navigator.getUserMedia = navigator.getUserMedia || navigator.webkitGetUserMedia ||
    navigator.mozGetUserMedia || navigator.msGetUserMedia || navigator.oGetUserMedia;

var video;
var video_stream;
var video_on = false;
var first_pass_on_file;
var canvas;
var ctx;
var salt_is_valid;
var entropy_is_valid;

// Compute key from salt and entropy: Key = SHA-256(salt || entropy).
function key() {
    var salt = sjcl.codec.hex.toBits($("#salt").val());
    var entropy = sjcl.codec.hex.toBits($("#entropy").val());
    var sha = sjcl.hash.sha256.hash(sjcl.bitArray.concat(salt, entropy));
    var sha_text = sjcl.codec.hex.fromBits(sha);
    // separate groups of 8 digits with spaces for readability
    $("#sha").text(sha_text.replace(/.{8}\B/g, "$& "));
    return sha;
}

$(function () {
    if (navigator.getUserMedia)
        $("#cambtn").attr("hidden", false);
    canvas = document.getElementById("qr-canvas");
    ctx = canvas.getContext("2d");

    // set up drag and drop
    var body = $("body")[0];
    body.ondragenter = function (e) {
        e.stopPropagation();
        e.preventDefault();
    };
    body.ondragover = body.ondragenter;
    body.ondrop = function (e) {
        e.stopPropagation();
        e.preventDefault();
        file_input(e.dataTransfer);
    };

    $("#calc-reg").click(function () {
        var pkey = [Number($("select").val())].concat(bits2array(key(), 256));
        if ($('input[type="checkbox"]')[0].checked)
            pkey.push(1);
        $("#privkey").text(b58check_encode(pkey));
        $("#address").text(bitcoin_address(pkey));
        $(".results-reg").show();
        $(".results-hd").hide();
        $("#results").show();
    });
    $("#calc-hd").click(function () {
        var m = new Mnemonic("english");
        m = m.toMnemonic(bits2array(key(), 128));
        $("#mnemonic").text(m);
        $(".results-hd").show();
        $(".results-reg").hide();
        $("#results").show();
    });
    $('input[type="text"], input[type="checkbox"], select').change(function (event) {
        $("#results").hide();
        if (event.target.id == "salt") {
            salt_is_valid = false;
            var len = event.target.value.replace(/\s|0x/g, "").length;
            if (len > 64) {
                $("#salt").parents(".form-group").addClass("has-error");
                $("#salt-too-long").show("fast");
                $("#salt-odd").hide("fast");
            } else if (len & 1) {
                $("#salt").parents(".form-group").addClass("has-error");
                $("#salt-odd").show("fast");
                $("#salt-too-long").hide("fast");
            } else {
                salt_is_valid = true;
                $("#salt").parents(".form-group").removeClass("has-error");
                $("#salt-too-long").hide("fast");
                $("#salt-odd").hide("fast");
            }
        } else if (event.target.id == "entropy") {
            var len = event.target.value.replace(/\s|0x/g, "").length;
            entropy_is_valid = len == 64;
            if (entropy_is_valid) {
                $("#entropy").parents(".form-group").removeClass("has-error");
                $("#entropy-wrong").hide("fast");
            } else {
                $("#entropy").parents(".form-group").addClass("has-error");
                $("#entropy-wrong").show("fast");
            }
        }
        var input_is_valid = salt_is_valid && entropy_is_valid && $(":invalid").length == 0;
        $("#calc-reg, #calc-hd").attr("disabled", !input_is_valid);
    });
});

function use_video() {
    $("#cambtn").attr("disabled", true);
    navigator.getUserMedia({video: true}, function (stream) {
        // handle video
        video_stream = stream;
        video = document.querySelector("video");
        document.getElementById("image").hidden = true;
        document.getElementById("video").hidden = false;
        $("#video button").attr("hidden", false);
        video.src = window.URL.createObjectURL(stream);
        video_on = true;
        canvas.width = 640;
        canvas.height = 480;
        setTimeout(decode_video, 2000);
    }, function (error) {
        // handle error
        console.log(error);
        $("#cambtn").attr("disabled", false);
    });
}

function decode_video() {
    console.log("dv: video paused " + video.paused + ", ended " + video.ended);
    if (!video_on) return;
    if (!video.paused && !video.ended) {
        try {
            var w = video.videoWidth;
            var h = video.videoHeight;
            if (w && h) {
                var scale = Math.ceil(Math.max(w / 800, h / 600));
                canvas.width = w / scale;
                canvas.height = h / scale;
                console.log("WxH, scale: " + w + " " + h + " " + scale);
            }
            ctx.drawImage(video, 0, 0, canvas.width, canvas.height);
            first_pass_on_file = false;
            qrcode.decode();
        } catch (e) {
            console.log(e);
        }
    }
    if (video_on)
        setTimeout(decode_video, 500);
}

function stop_video() {
    if (video_on) {
        video_on = false;
        video.pause();
        video_stream.stop();
        $("#video button").attr("hidden", true);
        $("#cambtn").attr("disabled", false);
    }
}

function clear_entropy() {
    entropy_is_valid = false;
    $("#entropy").val("").parents(".form-group").removeClass("has-error");
    $("#entropy-wrong").hide("fast");
    $("#calc-reg, #calc-hd").attr("disabled", true);
}

function file_input(dt) {
    if (dt.files && !dt.files[0]) return;
    stop_video();
    clear_entropy();

    $("#video")[0].hidden = true;
    $("#image")[0].hidden = false;

    first_pass_on_file = true;
    if (dt.getData && dt.getData("URL")) {
        $("img")[0].src = dt.getData("URL");
        qrcode.decode(dt.getData("URL"));
    } else if (dt.files) {
        var reader = new FileReader();
        reader.onload = function () {
            $("img")[0].src = reader.result;
            qrcode.decode(reader.result);
        }
        reader.readAsDataURL(dt.files[0]);
    }
}

qrcode.callback = function (text) {
    // validate text as Entropy-generated hex
    if ((text + " ").search(/^([0-9A-F]{8} ){8}$/) == 0) {
        $("#entropy").val(text).change();
        stop_video();
        return;
    }
    console.log(text);
    if (text.split(" ", 1) == "error") {
        // scan error, see if we can cut off the entropy part
        if (!first_pass_on_file) return;
        first_pass_on_file = false;
        if (cut_off_entropy_qr())
            qrcode.decode();
    } else {
        // scanned wrong QR code; vandalise it and start over
        for (var i = 0; i != qrcode.points.length; i++)
            erase_finder(qrcode.points[i], qrcode.moduleSize);
        qrcode.decode();
    }
}

function erase_finder(pos, size) {
    size *= 4.5;
    ctx.clearRect(pos.X - size, pos.Y - size, size * 2, size * 2);
}

function cut_off_entropy_qr() {
    var w = Math.ceil(canvas.width * 88 / 1808);
    var pix = ctx.getImageData(canvas.width - w, 0, w, canvas.height).data;

    for (var y = canvas.height; --y; ) {
        var sum = 0;
        for (var x = 0; x < w * 4; x++)
            if ((x & 3) != 3)
                sum += pix[y*w*4 + x];
        if (sum < 128 * 3 * w) {
            // found something that can be 'cut here'
            ctx.clearRect(0, 0, canvas.width, y - 1);
            return true;
        }
    }

    return false;
}

function set_image(img) {
    if (img.naturalWidth && img.naturalHeight) {
        var scale = Math.max(img.naturalWidth / 320, img.naturalHeight / 240, 1);
        img.width = img.naturalWidth / scale;
        img.height = img.naturalHeight / scale;
    }
}

function bits2array(src, len) {
    src = sjcl.bitArray.bitSlice(src, 0, len);
    var dst = [];
    for (var i = 0; i < len; i += 8) {
	dst[i/8] = sjcl.bitArray.extract(src, i, 8);
    }
    return dst;
}

function b58check_encode(bytes) {
    var bits = [];
    var w;
    for (var i = 0; i < bytes.length; i++) {
        w = i >> 2;
	bits[w] = (bits[w] || 0) << 8 | bytes[i];
    }
    if (bytes.length & 3) {
        bits[w] = sjcl.bitArray.partial(8 * (bytes.length & 3), bits[w]);
    }
    bits = sjcl.hash.sha256.hash(bits);
    bits = sjcl.hash.sha256.hash(bits);
    var csum = bits2array(bits, 32);
    return encode(new Uint8Array([].concat(bytes, csum)));
}

function secp256k1_G() {
    // p = 2^256 - 2^32 - 2^9 - 2^8 - 2^7 - 2^6 - 2^4 - 1
    var p = new BigInteger("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F", 16);
    var a = BigInteger.ZERO;
    var b = new BigInteger("7", 16);
    var curve = new ECCurveFp(p, a, b);
    var G = curve.decodePointHex("04"
          + "79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798"   // x
          + "483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8"); // y
    return G;
}

function bi_to_hex(x) {
    var h = x.toString(16);
    while (h.length < 64)
        h = "0" + h;
    return h;
}

function bitcoin_address(privkey) {
    var k = new BigInteger([0].concat(privkey.slice(1, 33)));
    var pub = secp256k1_G().multiply(k);
    var x = pub.getX().toBigInteger();
    var y = pub.getY().toBigInteger();
    if (privkey[33]) {
        // compressed public key
        pub = (y.testBit(0) ? "03" : "02") + bi_to_hex(x);
    } else {
        // uncompressed public key
        pub = "04" + bi_to_hex(x) + bi_to_hex(y);
    }
    pub = sjcl.hash.sha256.hash(sjcl.codec.hex.toBits(pub));
    pub = sjcl.hash.ripemd160.hash(pub);
    pub = [privkey[0] - 0x80].concat(bits2array(pub, 160));
    return b58check_encode(pub);
}
