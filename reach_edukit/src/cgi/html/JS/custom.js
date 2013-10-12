/*
* ENC1260函数库
*/

function initPageMenu() {
	//导航外观初始化
	var positionY = -42;
	var index = 3;
	if(jQuery.browser.msie) {
		positionY = -43;
	}
	
	var loginUserName = jQuery.cookies.get('user');
	if(loginUserName != 'admin') {
		index = 1;
	}
	//导航行为初始化
	initRollerAction();
	roller.init('wm-main-top', 'v', positionY, 0, 143, 20, index);
	
}

function showLeftMenuToSysInfo() {
	jQuery('#menu').hide();
	jQuery('#menu-right').css({
		"width": "965px",
		"margin-left": "15px"
	});
	jQuery('#maincontent').css({
		"width": "920px",
		"text-align": "center"
	});
}

function showLeftMenuFromSysInfo() {
	jQuery('#menu').show();
	jQuery('#menu-right').css({
		"width": "805px",
		"margin-left": "-2px"
	});
	jQuery('#maincontent').css({
		"width": "770px",
		"text-align": "left"
	});
}

function showleftMenuAnimate() {
	jQuery('#nav').hide();
	jQuery('#nav').show(500);
}

function showMainContentAnimate() {
	jQuery('#menu-right').animate({width:"toggle"}, 0);
	jQuery('#menu-right').animate({width:"toggle"}, 500);
}

function initRollerAction() {
	//加载系统信息页面
	jQuery('#nav_sysinfo').click(function() {
		if(!window.sysinfoRefreshTimestamp || (new Date().getTime() - window.sysinfoRefreshTimestamp) > 500) {
			window.sysinfoRefreshTimestamp = new Date().getTime();
			closeAllPrompt();
			jQuery('#wmform').html('');
			//roller.firstClicked = true;
			if((new Date().getTime()-roller.lastClickedTime) < 500) {
				return false;
			}
			roller.lastClickedTime = new Date().getTime();
			jQueryAjaxHtml({
				data: {"actioncode": "303"}
			});
			showLeftMenuToSysInfo();
			//showMainContentAnimate();
		} else {
			return false;
		}
	});	
	
	//加载文件管理菜单
	jQuery('#nav_filemgr').click(function() {
		if(!window.sysinfoRefreshTimestamp || (new Date().getTime() - window.sysinfoRefreshTimestamp) > 500) {
			window.sysinfoRefreshTimestamp = new Date().getTime();
			closeAllPrompt();
			jQuery('#wmform').html('');
			//roller.firstClicked = true;
			if((new Date().getTime()-roller.lastClickedTime) < 500) {
				return false;
			}
			roller.lastClickedTime = new Date().getTime();
			jQueryAjaxHtml({
				data: {"actioncode": "323"}
			});
			showLeftMenuToSysInfo();
			//showMainContentAnimate();
		} else {
			return false;
		}
	});	
	
	//加载参数设置菜单
	jQuery('#nav_paramset').click(function() {
		//roller.firstClicked = true;
		if((new Date().getTime()-roller.lastClickedTime) < 500) {
			return false;
		}
		roller.lastClickedTime = new Date().getTime();
		jQueryAjaxHtml({
			data: {"actioncode": "301"},
			renderToID: 'nav'
		});
		showLeftMenuFromSysInfo();
		showleftMenuAnimate();
	});
	
	//加载系统设置菜单
	jQuery('#nav_sysset').click(function() {
		//roller.firstClicked = true;
		if((new Date().getTime()-roller.lastClickedTime) < 500) {
			return false;
		}
		roller.lastClickedTime = new Date().getTime();
		jQueryAjaxHtml({
			data: {"actioncode": "302"},
			renderToID: 'nav'
		});
		showLeftMenuFromSysInfo();
		showleftMenuAnimate();
	});
	
	//点击第一个导航
	var loginUserName = jQuery.cookies.get('user');
	if(loginUserName != 'admin') {
		jQuery('#wm-main-top li:not(".filemgrLi")').remove();
		jQuery('#nav_filemgr').click();
	} else {
		jQuery('#nav_paramset').click();
	}
	
}

/*
 * 初始化透明度滑动条
 * */
function initBrightnessSlider() {
	window.cap_sliderbar = new Slider("cap_bright_slider", "cap_bright_bar", {
		onMove: function(){
			if(navigator.userAgent.toLowerCase().indexOf('msie 8') != -1) {
				O("cap_brightness").value = Math.round(this.GetValue()) - 1;
			} else {
				O("cap_brightness").value = Math.round(this.GetValue());
			}
			
		}
	});
	
	jQuery('#cap_brightness').change(function() {
		var value = O("cap_brightness").value;
		if(isNaN(value)) {
			value = 100;
		}
		cap_sliderbar.SetValue(parseInt(value, 10));
	});
	
	window.logo_sliderbar = new Slider("logo_bright_slider", "logo_bright_bar", {
		onMove: function(){
			if(navigator.userAgent.toLowerCase().indexOf('msie 8') != -1) {
				O("logo_brightness").value = Math.round(this.GetValue()) - 1;
			} else {
				O("logo_brightness").value = Math.round(this.GetValue());
			}
			
		}
	});
	jQuery('#logo_brightness').change(function() {
		var value = O("logo_brightness").value;
		if(isNaN(value)) {
			value = 100;
		}
		logo_sliderbar.SetValue(parseInt(value, 10));
	});
	
}

/*
 * 修正表单赋值后滑动条没反应的问题
 */
function fixBrightnessSlider() {
	var value = O("cap_brightness").value;
	if(isNaN(value)) {
		value = 100;
	}
	cap_sliderbar.SetValue(parseInt(value, 10));
	
	var value = O("logo_brightness").value;
	if(isNaN(value)) {
		value = 100;
	}
	logo_sliderbar.SetValue(parseInt(value, 10));
}

/*
 * 表单美化
 * */
function formBeautify() {
	zebraTransform.update();
}


/*
 * 初始化抬头信息
 * */
function initTopInfo() {
	var loginUserName = jQuery.cookies.get('user');
	if(loginUserName) {
		jQuery('#loginUserName').html(loginUserName);
	}
	jQuery('#logoutLink').click(function() {
		jQueryAjaxCmd({
			data: {"actioncode": "200"},
			success: function(ret) {
				eval(ret);
			}
		});
	});
}


/*
 * 显示和隐藏uploading提示层
 */
function openUploading() {
	jQuery('#uploadingDiv').width(document.body.scrollWidth);
	jQuery('#uploadingDiv').height(document.body.scrollHeight);
	jQuery('#uploadingDiv').show();
}

function closeUploading() {
	jQuery('#uploadingDiv').hide();
}




