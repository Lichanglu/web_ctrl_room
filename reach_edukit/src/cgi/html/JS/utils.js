
/*
* ENC1260���߿�
*/


/*
 * ��ȡhtmlҳ������� 
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
 * ��CGI��GET���������
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
 * ��CGI��POST���������
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
 * ����֤
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
 * ����ֵ������װ
 * */
var formSet = function() {
	return {
		//Ϊ�ı���ֵ
		setTextValue: function(formID, name, value) {
			if(value != 'undefined') {
				jQuery(formID + ' [name="' + name + '"]:text').val(value);
			}
		},
		
		//��ȡ�ı����ֵ
		getTextValue: function(formID, name) {
			return jQuery(formID + ' [name="' + name + '"]:text').val();
		},
		
		//Ϊ������ֵ
		setHiddenValue: function(formID, name, value) {
			if(value != 'undefined') {
				jQuery(formID + ' [name="' + name + '"]:hidden').val(value);
			}
		},
		
		//��ȡ�������ֵ
		getHiddenValue: function(formID, name) {
			return jQuery(formID + ' [name="' + name + '"]:hidden').val();
		},
		
		//Ϊ������ֵ
		setSelectValue: function(formID, name, value) {
			if(value != 'undefined') {
				jQuery(formID + ' select[name="' + name + '"]').val(value+"");
			}
		},
		
		//��ȡ�������ֵ
		getSelectValue: function(formID, name) {
			return jQuery(formID + ' select[name="' + name + '"]').val();
		},
		
		//Ϊ��ѡ��ֵ
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
		
		//��ȡ��ѡ���ֵ
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
		
		//ѡ��ĳһ��ѡ��
		setCheckBoxValue: function(formID, name, value) {
			if(value != 'undefined') {
				if(value == '1') {
					jQuery(formID + ' [name="' + name + '"]:checkbox').attr("checked", true);
				} else {
					jQuery(formID + ' [name="' + name + '"]:checkbox').removeAttr("checked");
				}
			}
		},
		
		//��ȡ��ѡ���״̬
		getCheckBoxValue: function(formID, name) {
			if(jQuery(formID + ' [name="' + name + '"]:checkbox').attr("checked")) {
				return 1;
			} else {
				return 0;
			}
		},
		
		//��ȡ�ı����ֵ
		getTextAreaValue: function(formID, name) {
			return jQuery(formID + ' textarea[name="' + name + '"]').val();
		},
		
		//Ϊ�ı���ֵ
		setTextAreaValue: function(formID, name, value) {
			if(value != 'undefined') {
				jQuery(formID + ' textarea[name="' + name + '"]').val(value);
			}
		},
	};
}();


/*
 * ����ֵ
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
 * ��ȡ��ֵ
 */
function getFormItemValue(formID) {
	var result = {};
	if(!formID) {
		return result;
	} else {
		formID = '#' + formID + ' ';
	}
	//�ı���
	jQuery(formID + 'input:text').each(function() {
		var jQueryThis = jQuery(this);
		var name = jQueryThis.attr('name');
		var value = jQueryThis.val();
		result[name] = value;
	});
	
	//������
	jQuery(formID + 'input:password').each(function() {
		var jQueryThis = jQuery(this);
		var name = jQueryThis.attr('name');
		var value = jQueryThis.val();
		result[name] = value;
	});
	
	//������
	jQuery(formID + 'input:hidden').each(function() {
		var jQueryThis = jQuery(this);
		var name = jQueryThis.attr('name');
		var value = jQueryThis.val();
		result[name] = value;
	});
	
	//������
	jQuery(formID + 'select').each(function() {
		var jQueryThis = jQuery(this);
		var name = jQueryThis.attr('name');
		var value = jQueryThis.val();
		result[name] = value;
	});
	
	//��ѡ��
	jQuery(formID + 'input:radio').each(function() {
		var jQueryThis = jQuery(this);
		if(jQueryThis.attr('checked')) {
			var name = jQueryThis.attr('name');
			var value = jQueryThis.val();
			result[name] = value;
		}
	});
	
	//��ѡ��
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








