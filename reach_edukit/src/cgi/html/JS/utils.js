
/*
* ENC1260工具库
*/


/*
 * 获取html页面的请求 
 * */
 function jQueryAjaxHtml(options) {
	var defaults = {
		url: "encoder.cgi",
		dataType: "html",
		data: {},
		cache: false,
		renderToID: "maincontent",
		effectsOn:false,
		success: function(retHTML){
			if(this.renderToID) {
				jQuery('#' + this.renderToID).html(retHTML);
			}
			if(this.effectsOn){
				jQuery('.ui-tabs-inner-content').fadeOut("fast");
				jQuery('.ui-tabs-inner-content').fadeIn("fast");
			}
		}
	};
	
	if(options) {
		jQuery.extend(defaults, options);
	}
	jQuery.ajax(defaults);
}

/*
 * 向CGI发GET命令的请求
 * */
function jQueryAjaxCmd(options) {
	var defaults = {
		url: "encoder.cgi",
		cache: false,
		data: {},
		success: function(ret){}
	};
	
	if(options) {
		jQuery.extend(defaults, options);
	}
	
	jQuery.ajax(defaults);
}

/*
 * 向CGI发POST命令的请求
 * add by tanqh
 * */
function jQueryPostAjaxCmd(options) {
	var defaults = {
		url: "encoder.cgi",
		type: "POST",
		cache: false,
		data: {},
		success: function(ret){}
	};
	
	if(options) {
		jQuery.extend(defaults, options);
	}
	
	jQuery.ajax(defaults);
}

/*
 * 表单验证
 */
function initFormValidation() {
	jQuery("form").validationEngine({'validationEventTrigger': ''});
}

function closeAllPrompt() {
	jQuery('form').validationEngine('hideAll');
}

function validateFormByID(formID) {
	return jQuery('#' + formID).validationEngine('validate');
}

/*
 * 表单赋值方法封装
 * */
var formSet = function() {
	return {
		//为文本框赋值
		setTextValue: function(formID, name, value) {
			if(value != 'undefined') {
				jQuery(formID + ' [name="' + name + '"]:text').val(value);
			}
		},
		
		//获取文本框的值
		getTextValue: function(formID, name) {
			return jQuery(formID + ' [name="' + name + '"]:text').val();
		},
		
		//为隐藏域赋值
		setHiddenValue: function(formID, name, value) {
			if(value != 'undefined') {
				jQuery(formID + ' [name="' + name + '"]:hidden').val(value);
			}
		},
		
		//获取隐藏域的值
		getHiddenValue: function(formID, name) {
			return jQuery(formID + ' [name="' + name + '"]:hidden').val();
		},
		
		//为下拉框赋值
		setSelectValue: function(formID, name, value) {
			if(value != 'undefined') {
				jQuery(formID + ' select[name="' + name + '"]').val(value+"");
			}
		},
		
		//获取下拉框的值
		getSelectValue: function(formID, name) {
			return jQuery(formID + ' select[name="' + name + '"]').val();
		},
		
		//为单选框赋值
		setRadioValue: function(formID, name, value) {
			if(value != 'undefined') {
				jQuery(formID + ' [name="' + name + '"]:radio').each(function() {
					var jQueryThis = jQuery(this);
					if(jQueryThis.val() == value) {
						jQueryThis.attr('checked', true);
					}
				});
			}
		},
		
		//获取单选框的值
		getRadioValue: function(formID, name) {
			var retvalue = 0;
			jQuery(formID + ' [name="' + name + '"]:radio').each(function() {
				var jQueryThis = jQuery(this);
				if(jQueryThis.prop('checked')) {
					retvalue =  jQueryThis.val();
					return;
				}
			});
			return retvalue;
		},
		
		//选中某一复选框
		setCheckBoxValue: function(formID, name, value) {
			if(value != 'undefined') {
				if(value == '1') {
					jQuery(formID + ' [name="' + name + '"]:checkbox').attr("checked", true);
				} else {
					jQuery(formID + ' [name="' + name + '"]:checkbox').removeAttr("checked");
				}
			}
		},
		
		//获取复选框的状态
		getCheckBoxValue: function(formID, name) {
			if(jQuery(formID + ' [name="' + name + '"]:checkbox').attr("checked")) {
				return 1;
			} else {
				return 0;
			}
		},
		
		//获取文本域的值
		getTextAreaValue: function(formID, name) {
			return jQuery(formID + ' textarea[name="' + name + '"]').val();
		},
		
		//为文本域赋值
		setTextAreaValue: function(formID, name, value) {
			if(value != 'undefined') {
				jQuery(formID + ' textarea[name="' + name + '"]').val(value);
			}
		},
	};
}();


/*
 * 表单赋值
 */
function setFormItemValue(formID, keyValueArray) {
	//jQuery('#changeLanguage').val(1);
	if(keyValueArray && typeof keyValueArray == 'object') {
		if(!formID) {
			formID = '';
		} else {
			formID = '#' + formID + ' ';
		}
		for(var i=0; i<keyValueArray.length; i++) {
			var keyvalueObj = keyValueArray[i];
			if(keyvalueObj && typeof keyvalueObj == 'object') {
				var name = keyvalueObj['name'];
				var value = keyvalueObj['value'];
				var type = keyvalueObj['type'];
				if(name && value && type){
					switch(type) {
						case 'radio': {
							formSet.setRadioValue(formID, name, value);
							break;
						}
						case 'checkbox': {
							formSet.setCheckBoxValue(formID, name, value);
							break;
						}
						case 'select': {
							formSet.setSelectValue(formID, name, value);
							break;
						}
						case 'hidden': {
							formSet.setHiddenValue(formID, name, value);
						}
						case 'textarea': {
							formSet.setTextAreaValue(formID, name, value);
						}
						default: {
							formSet.setTextValue(formID, name, value);
							break;
						}
					}
				}
			}
		}
	}
}

/*
 * 获取表单值
 */
function getFormItemValue(formID) {
	var result = {};
	if(!formID) {
		return result;
	} else {
		formID = '#' + formID + ' ';
	}
	//文本域
	jQuery(formID + 'input:text').each(function() {
		var jQueryThis = jQuery(this);
		var name = jQueryThis.attr('name');
		var value = jQueryThis.val();
		result[name] = value;
	});
	
	//密码域
	jQuery(formID + 'input:password').each(function() {
		var jQueryThis = jQuery(this);
		var name = jQueryThis.attr('name');
		var value = jQueryThis.val();
		result[name] = value;
	});
	
	//隐藏域
	jQuery(formID + 'input:hidden').each(function() {
		var jQueryThis = jQuery(this);
		var name = jQueryThis.attr('name');
		var value = jQueryThis.val();
		result[name] = value;
	});
	
	//下拉框
	jQuery(formID + 'select').each(function() {
		var jQueryThis = jQuery(this);
		var name = jQueryThis.attr('name');
		var value = jQueryThis.val();
		result[name] = value;
	});
	
	//单选框
	jQuery(formID + 'input:radio').each(function() {
		var jQueryThis = jQuery(this);
		if(jQueryThis.attr('checked')) {
			var name = jQueryThis.attr('name');
			var value = jQueryThis.val();
			result[name] = value;
		}
	});
	
	//复选框
	jQuery(formID + 'input:checkbox').each(function() {
		var jQueryThis = jQuery(this);
		var name = jQueryThis.attr('name');
		var value = 0;
		if(jQueryThis.attr('checked')) {
			value = 1;
		}
		result[name] = value;
	});
	
	for(var name in result) {
		alert(name + ":" + result[name]);
	}
	return result;
}


function falert(pokValue, pcontent) {
	art.dialog({
		title: '',
		okValue: pokValue + '',
		lock: true,
		fixed: true,
	    background: '#600', 
	    opacity: 0.87,
	    content: pcontent + '',
	    ok: true
	});
}








