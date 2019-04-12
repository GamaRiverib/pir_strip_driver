var url="/rpc/",data=null,PAGES={CONNECTING:"index.html",DASHBOARD:"dashboard.html",MENU:"menu.html"},EVENTS={RPC_CALLBACK:"rpcCb"},MODES=["OFF","LIGHT ON","EFFECT LIGHTS"],loading_mask=document.getElementById("loading_mask"),loading_msg=document.getElementsByClassName("loading")[0],submit_btn=document.getElementById("submit_btn");let back_btns=document.getElementsByClassName("back_btn");for(let t=0;t<back_btns.length;t++)back_btns[t].onclick=function(t){t.preventDefault(),window.history.back()};let home_btns=document.getElementsByClassName("home_btn");for(let t=0;t<home_btns.length;t++)home_btns[t].onclick=function(t){return t.preventDefault(),window.location.href=PAGES.DASHBOARD};function showMask(t){loading_mask&&(loading_msg.innerHTML=t,loading_mask.hidden=!1)}function hideMask(){loading_mask&&(loading_msg.innerHTML="",loading_mask.hidden=!0)}function getJson(t,e,n){let a=new XMLHttpRequest;function o(t){hideMask(),e({error:t})}a.open("GET",t,!0),a.onreadystatechange=function(){4===this.readyState&&this.status>=200&&this.status<400&&e(null,JSON.parse(this.response))},a.ontimeout=o,a.onerror=o,a.onloadend=hideMask,showMask(n),a.send()}function postJson(t,e,n,a){let o=new XMLHttpRequest;function i(t){hideMask(),n({error:t})}o.open("POST",t,!0),o.onreadystatechange=function(){4===this.readyState&&this.status>=200&&this.status<400&&n(null,JSON.parse(this.response))},o.ontimeout=i,o.onerror=i,o.onloadend=hideMask,showMask(a),o.send(e)}function rpc(t){t.preventDefault();let e=t.target.getAttribute("href").substr(1);getJson(url+e,function(e,n){if(null!==e)return void alert("Error trying to call remote procedure.");let a=new CustomEvent(EVENTS.RPC_CALLBACK,{response:n});t.target.dispatchEvent(a)},"Setting device state...")}function sHbC(t,e){let n=document.getElementsByClassName(t);for(let t=0;t<n.length;t++)n[t].innerHTML=e}function sIVById(t,e){let n=document.getElementById(t);n&&("boolean"==typeof e?n.checked=e:n.value=e,n.onchange=function(n){let a=n.target.value;if("number"==typeof e&&(a=parseInt(a)),a!=e)null===data&&(data={}),data[t]="boolean"==typeof e?n.target.checked:a,submit_btn.removeAttribute("disabled");else{data&&data[t]&&delete data[t];let e=0;for(let t in data)e++;0===e&&submit_btn.setAttribute("disabled",!0)}})}function sC(t,e){if("object"==typeof e)for(let n in e)sIVById(t+"."+n,e[n])}submit_btn&&(submit_btn.onclick=function(t){t.preventDefault(),null!==data&&postJson(url+"Config.Set",JSON.stringify({config:data}),function(t,e){t?alert("Error sending the new configuration. Try again later."):postJson(url+"Config.Save",JSON.stringify({reboot:!0}),function(t,e){t?alert("Error saving the new configuration. Try again later."):(data=null,submit_btn.setAttribute("disabled",!0),alert("Device is restarted. Please wait a moment until you connect again."),location.reload())},"Saving configuration...")},"Sending configuration...")});