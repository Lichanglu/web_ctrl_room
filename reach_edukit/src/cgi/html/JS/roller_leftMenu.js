//导航
var roller = function () {
	return {
		init:function (el, ty, sx, ex, d, st, index) {
			roller.containerID = el;
			roller.lastClickedTime = 0;
			var e = document.getElementById(roller.containerID);
			if (!e) {
				return;
			}
			var a = e.getElementsByTagName("a");
			for (i = 0; i < a.length; i++) {
				if (!a[i].id) {
					a[i].id = el.id + i;
				}
				a[i].n = a[i].o = sx;
				a[i].en = ex;
				a[i].ty = ty;
				if (a[i].ty == "v") {
					a[i].style.backgroundPosition = "0px " + a[i].n + "px";
				} else {
					if (a[i].ty == "h") {
						a[i].style.backgroundPosition = a[i].n + "px 0px";
					} else {
						return;
					}
				}
				a[i].onmouseover = roller.o;
				a[i].onmouseout = roller.o;
				a[i].onclick = roller.o;
				a[i].st = Math.abs(Math.abs(ex - sx) / st);
				a[i].t = d / st;
			}
			
			if(index) {
				var e = document.getElementById(roller.containerID);
				if (!e) {
					return;
				}
				var a = e.getElementsByTagName("a");
				for (i = 0; i < a.length; i++) {
					if(index == (i+1)) {
						a[i].w = a[i].en;
						a[i].isClicked = true;
						roller.s(a[i]);	
					}
				}
			}
			
		},
		 
		o:function (e) {
			var evt = e || window.event;
			c = evt.target != null ? evt.target : evt.srcElement;
			if (c.nodeName == "A" && evt.type == "mouseover") {
				c.w = c.en;
				roller.s(c);
			} else {
				if (c.nodeName == "A" && evt.type == "click") {
					if((new Date().getTime()-roller.lastClickedTime) < 500) {
						return false;
					}
					var ele = document.getElementById(roller.containerID);
					if (!ele) {
						return;
					}
					var a = ele.getElementsByTagName("a");
					for (i = 0; i < a.length; i++) {
						a[i].isClicked = false;
						a[i].w = a[i].o;
						roller.s(a[i]);
					}
					c.w = c.en;
					c.isClicked = true;
					roller.s(c);
				} else {
					if (c.nodeName == "A") {
						if (c.isClicked) {
							return;
						}
						c.w = c.o;
						roller.s(c);
					}
				}
			}
		}, 
		s:function (e) {
			if (e.ti) {
				clearTimeout(e.ti);
			}
			if (Math.abs(e.n - e.w) < e.st) {
				e.n = e.w;
			} else {
				if (e.n < e.w) {
					e.n = e.n + e.st;
				} else {
					if (e.n > e.w) {
						e.n = e.n - e.st;
					}
				}
			}
			if (e.ty == "v") {
				e.style.backgroundPosition = "0px " + e.n + "px";
			} else {
				e.style.backgroundPosition = e.n + "px 0px";
			}
			if (e.n == e.w) {
				clearTimeout(e.ti);
				return;
			}
			e.ti = setTimeout(function () {
				roller.s(e);
			}, e.t);
		}
		
	};
}();


//左边菜单
var leftMenu = function() {
	return {
		init: function(conID, selIndex) {
			leftMenu.containerID = conID;
			// mousehover and mouseout event.
			jQuery('#' + leftMenu.containerID + ' li a').hover(function(evt) {
				var jQuerythis = jQuery(this);
				jQuerythis.addClass('menu' + jQuerythis.prop('id').substr(4));
			}, function(evt) {
				var jQuerythis = jQuery(this);
				// return if the target was clicked.
				if(jQuerythis.prop('class').indexOf('clicked') != -1) {
					return false;
				}
				jQuerythis.removeClass('menu' + jQuerythis.prop('id').substr(4));
			});
			
			// mouseclick event.
			jQuery('#' + leftMenu.containerID + ' li a').click(function(evt) {
				var jQuerythis = jQuery(this);
				var suffix = jQuerythis.prop('id').substr(4);
				if(leftMenu.currSuffix) {
					jQuery('.menu' + leftMenu.currSuffix).removeClass('menu' + leftMenu.currSuffix).removeClass('clicked');
				}
				jQuerythis.addClass('menu' + suffix).addClass('clicked');
				leftMenu.currSuffix = suffix;
			});
			
			if(selIndex) {
				jQuery('#' + leftMenu.containerID + ' li a').eq(selIndex-1).click();
			}
		}
	}
}();

