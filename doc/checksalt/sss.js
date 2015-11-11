"use strict";

navigator.getUserMedia = navigator.getUserMedia || navigator.webkitGetUserMedia ||
    navigator.mozGetUserMedia || navigator.msGetUserMedia || navigator.oGetUserMedia;

var video;
var video_stream;
var video_constraints = true;
var video_on = false;
var first_pass_on_file;
var canvas;
var ctx;
var key_qr;

var shares = [];
var set_id;
var threshold;

// Combine shares and compute the secret.
function combine() {
    // We use the same Galois field as in QR codes.
    var GF = GF256.QR_CODE_FIELD;
    // Prepare Lagrange numerator.
    var num = shares.reduce(function (n, s, x) {
        return GF.multiply(n, x);
    }, 1);

    var secret = shares.reduce(function (a, share, x) {
        // Prepare Lagrange denominator.
        var den = shares.reduce(function (d, share, xj) {
            return xj == x ? d : GF.multiply(d, x ^ xj);        // den *= xj - x
        }, x);
        // Compute Lagrange coefficient lc = num / den.
        var lc = GF.multiply(num, GF.inverse(den));

        var term = new GF256Poly(GF, share).multiply2(lc);
        return a.addOrSubtract(term);
    }, GF.Zero);

    // GF256Poly omits leading zeroes (as it goes from higher degree to lower).
    // But we know that the first byte is WIF prefix, which is never 0.

    return secret.Coefficients;
}

function show_key() {
    var pkey = combine();
    var key_text = b58check_encode(pkey);
    $("#privkey").text(key_text);
    $("#address").text(bitcoin_address(pkey));
    $("#private-key").show();
    key_qr.makeCode(key_text);
}

function add_share(share_text) {
    $("#share-wrong").hide("fast");
    $("#share-incompatible").hide("fast");
    $("#share").parents(".form-group").removeClass("has-error");

    if (share_text == "")
        return;

    var share = b58check_decode(share_text.slice(4));
    if (share_text.slice(0, 4) != "SSS-" || share.len < 4 + 33) {
        $("#share").parents(".form-group").addClass("has-error");
        $("#share-wrong").show("fast");
        return;
    }

    var id = share[1] << 8 | share[2];
    var thr = (share[3] >> 4) + 1;
    var x = (share[3] & 15) + 1;

    if (share[0] != 19 || shares.length > 0 && (set_id != id || threshold != thr)) {
        $("#share").parents(".form-group").addClass("has-error");
        $("#share-incompatible").show("fast");
        return;
    }

    if (shares[x])
        return;         // already exists

    if (shares.length == 0) {
        set_id = id;
        threshold = thr;
        $("#set-id").text(("000" + id.toString(16)).slice(-4));
        $("#threshold").text(thr);
        $("#shares").show("fast");
    }

    shares[x] = share.slice(4);

    $("#list-of-shares").append("#" + x + ": <code>" + share_text + "</code><br>");
    $("#share").val("");

    // Compute the number of elements in a sparse array.
    var available = shares.reduce(function (n) { return n + 1; }, 0);
    if (available == threshold)
        show_key();
}

function clear_shares(e) {
    shares = [];
    $("#list-of-shares").text("");
    $("#shares, #private-key").hide("slow");
    e.stopPropagation();
    e.preventDefault();
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

    $("#share").change(function (event) {
        var share_text = event.target.value.trim();
        add_share(share_text);
    });
    $("#clear-shares").click(clear_shares);
    $("#video button").click(stop_video);

    key_qr = new QRCode("key-qr", {
        width: 132,
        height: 132,
        correctLevel: QRCode.CorrectLevel.L
    });
});

if (this.MediaStreamTrack) {
    MediaStreamTrack.getSources(function (src) {
        for (var i in src) {
            if (src[i].kind == "video" && src[i].facing == "environment") {
                video_constraints = { optional: [{ sourceId: src[i].id }] };
                break;
            }
        }
    });
}

function use_video() {
    $("#cambtn").attr("disabled", true);
    if (video_constraints != true)
        $("video").css("transform", "none");
    navigator.getUserMedia({ video: video_constraints }, function (stream) {
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

function stop_video(e) {
    if (video_on) {
        video_on = false;
        video.pause();
        video_stream.stop();
        $("#video button").attr("hidden", true);
        $("#cambtn").attr("disabled", false);
    }
    if (e) {
        e.stopPropagation();
        e.preventDefault();
    }
}

function file_input(dt) {
    if (dt.files && !dt.files[0]) return;
    stop_video();

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
    // validate text as a private key share
    if (text.slice(0, 4) == "SSS-") {
        $("#share").val(text).change();
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

function b58check_decode(b58string) {
    var bytes = decode(b58string);
    var plen = bytes.length - 4;        // payload length
    if (plen <= 0) return [];

    var bits = [];
    var w;
    for (var i = 0; i < plen; i++) {
        w = i >> 2;
	bits[w] = (bits[w] || 0) << 8 | bytes[i];
    }
    if (plen & 3) {
        bits[w] = sjcl.bitArray.partial(8 * (bytes.length & 3), bits[w]);
    }
    bits = sjcl.hash.sha256.hash(bits);
    bits = sjcl.hash.sha256.hash(bits);
    var csum = bits2array(bits, 32);
    if (bytes.slice(plen).toString() != csum.toString())
        return [];
    return bytes.slice(0, plen);
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
