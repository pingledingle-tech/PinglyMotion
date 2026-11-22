// Language picker and light/dark switch for every page of the website.
// Texts that change per page come from the JSON block with id "i18n" in the page.
(function(){
var M=JSON.parse(document.getElementById("i18n").textContent),L=["en","no","de","pl","nl"];
function ok(c){return L.indexOf(c)>=0;}
function pick(){
  var h=location.hash.slice(1);if(ok(h))return h;
  try{var s=localStorage.getItem("pinglymotion-lang");if(ok(s))return s;}catch(e){}
  var n=navigator.languages&&navigator.languages.length?navigator.languages:[navigator.language||""];
  for(var i=0;i<n.length;i++){var c=String(n[i]).toLowerCase().split("-")[0];if(c==="nb"||c==="nn")c="no";if(ok(c))return c;}
  return "en";
}
function show(c,store){
  var m=M[c],a=document.getElementsByTagName("article");
  for(var i=0;i<a.length;i++){a[i].className=a[i].id===c?"on":"";}
  document.documentElement.lang=m.lang;document.title=m.title;
  var d=document.querySelector('meta[name="description"]');if(d)d.setAttribute("content",m.desc);
  var k=document.querySelectorAll(".langs a");
  for(var j=0;j<k.length;j++){if(k[j].getAttribute("href")==="#"+c)k[j].setAttribute("aria-current","true");else k[j].removeAttribute("aria-current");}
  var s=document.querySelector(".skip");s.textContent=m.skip;s.setAttribute("href","#"+c);s.setAttribute("lang",m.lang);
  document.getElementById("theme-label").textContent=m.theme;
  var b=document.querySelectorAll("[data-theme-choice]");
  for(var t=0;t<b.length;t++){b[t].textContent=m[b[t].getAttribute("data-theme-choice")];}
  if(store){try{localStorage.setItem("pinglymotion-lang",c);}catch(e){}}
}
var dark=window.matchMedia?window.matchMedia("(prefers-color-scheme: dark)"):null;
function theme(){var t=document.documentElement.getAttribute("data-theme");return t||(dark&&dark.matches?"dark":"light");}
function marks(){var t=theme(),b=document.querySelectorAll("[data-theme-choice]");
  for(var i=0;i<b.length;i++){b[i].setAttribute("aria-pressed",b[i].getAttribute("data-theme-choice")===t?"true":"false");}}
function setTheme(t){document.documentElement.setAttribute("data-theme",t);try{localStorage.setItem("pinglymotion-theme",t);}catch(e){}marks();}
var tb=document.querySelectorAll("[data-theme-choice]");
for(var q=0;q<tb.length;q++){tb[q].addEventListener("click",function(){setTheme(this.getAttribute("data-theme-choice"));});}
if(dark){var f=function(){marks();};if(dark.addEventListener)dark.addEventListener("change",f);else if(dark.addListener)dark.addListener(f);}
marks();
var c=pick();show(c,false);
if(location.hash){window.addEventListener("load",function(){setTimeout(function(){window.scrollTo(0,0);},0);});}
if(location.hash.slice(1)!==c&&history.replaceState){try{history.replaceState(null,"","#"+c);}catch(e){}}
window.addEventListener("hashchange",function(){var h=location.hash.slice(1);if(ok(h)){show(h,true);window.scrollTo(0,0);}});
})();
