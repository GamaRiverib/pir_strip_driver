var url = "/rpc/";
var data = null;

var PAGES = {
    CONNECTING: "index.html",
    DASHBOARD: "dashboard.html",
    MENU: "menu.html"
};

var EVENTS = {
    RPC_CALLBACK: "rpcCb"
};

var MODES = ["OFF", "LIGHT ON", "EFFECT LIGHTS", "NIGHT LIGHT", "VIGILANCE"];

var loading_mask = document.getElementById("loading_mask");
var loading_msg = document.getElementsByClassName("loading")[0];
var submit_btn = document.getElementById("submit_btn");

let back_btns = document.getElementsByClassName("back_btn");
for(let i = 0; i < back_btns.length; i++) {
    back_btns[i].onclick = function(e) {
        e.preventDefault();
        window.history.back();
    };
}
let home_btns = document.getElementsByClassName("home_btn");
for(let i = 0; i < home_btns.length; i++) {
    home_btns[i].onclick = function(e) {
        e.preventDefault();
        return window.location.href = PAGES.DASHBOARD;
    }
}

function showMask(msg) {
    if(loading_mask) {
        loading_msg.innerHTML = msg;
        loading_mask.hidden = false;
    }
}

function hideMask() {
    if(loading_mask) {
        loading_msg.innerHTML = "";
        loading_mask.hidden = true;
    }
}

function getJson(url, cb, msg) {
    let xhttp = new XMLHttpRequest();
    xhttp.open("GET", url, true);
    xhttp.onreadystatechange = function() {
        if(this.readyState === 4) {
            if(this.status >= 200 && this.status < 400) {
                cb(null, JSON.parse(this.response));
            }
        }
    };
    function errorHandler(ev) {
        hideMask();
        cb({error: ev});
    }
    xhttp.ontimeout = errorHandler;
    xhttp.onerror = errorHandler;
    xhttp.onloadend = hideMask;
    showMask(msg);
    xhttp.send();
}

function postJson(url, data, cb, msg) {
    let xhttp = new XMLHttpRequest();
    xhttp.open("POST", url, true);
    xhttp.onreadystatechange = function() {
        if(this.readyState === 4) {
            if(this.status >= 200 && this.status < 400) {
                cb(null, JSON.parse(this.response));
            }
        }
    };
    function errorHandler(ev) {
        hideMask();
        cb({error: ev});
    }
    xhttp.ontimeout = errorHandler;
    xhttp.onerror = errorHandler;
    xhttp.onloadend = hideMask;
    showMask(msg);
    xhttp.send(data);
}

function rpc(e) {
    e.preventDefault();
    let m = e.target.getAttribute("href").substr(1);
    getJson(url + m, function(err, resp) {
        if (err !== null) {
            alert("Error trying to call remote procedure.");
            return;
        }
        let event = new CustomEvent(EVENTS.RPC_CALLBACK, { response: resp });
        e.target.dispatchEvent(event);
    }, "Setting device state...");
}

function sHbC(clazz, val) {
    let a = document.getElementsByClassName(clazz);
    for(let i = 0; i < a.length; i++) {
        a[i].innerHTML = val;
    }
}

function sIVById(id, value) {
    let f = document.getElementById(id);
    if (f) {
        if(typeof(value) === "boolean") {
            f.checked = value;
        } else {
            f.value = value;
        }
        f.onchange = function(ev) {
            let nv = ev.target.value;
            if(typeof(value) === "number") {
                nv = parseInt(nv);
            }
            if(nv == value) {
                if(data && data[id]) {
                    delete data[id];
                }
                let count = 0;
                for(let k in data) {
                    count++;
                }
                if (count === 0) {
                    submit_btn.setAttribute("disabled", true);
                }
                return;
            }
            if (data === null) {
                data = { };
            }
            if(typeof(value) === "boolean") {
                data[id] = ev.target.checked;
            } else {
                data[id] = nv;
            }
            submit_btn.removeAttribute("disabled");
        };
    }
}

function sC(config, values) {
    if(typeof(values) === "object") {
        for(let k in values) {
            sIVById(config + "." + k, values[k]);
        }
    }
}

if (submit_btn) {
    submit_btn.onclick = function(ev) {
        ev.preventDefault();
        if (data !== null) {
            postJson(url + "Config.Set", JSON.stringify({ config: data }), function(err, resp) {
                if(err) {
                    alert("Error sending the new configuration. Try again later.");
                    return;
                }
                postJson(url + "Config.Save", JSON.stringify({"reboot": true}), function(err1, resp1) {
                    if(err1) {
                        alert("Error saving the new configuration. Try again later.");
                        return;
                    }
                    data = null;
                    submit_btn.setAttribute("disabled", true);
                    alert("Device is restarted. Please wait a moment until you connect again.");
                    location.reload();
                }, "Saving configuration...");
            }, "Sending configuration...");
        }
    };
}