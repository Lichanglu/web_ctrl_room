
var isIE = (document.all) ? true : false;

var O = function (id) {
	return "string" == typeof id ? document.getElementById(id) : id;
};

var Class = {
	create: function() {
		return function() { this.initialize.apply(this, arguments); }
	}
}

var Extend = function(destination, source) {
	for (var property in source) {
		destination[property] = source[property];
	}
}

var Bind = function(object, fun) {
	var args = Array.prototype.slice.call(arguments).slice(2);
	return function() {
		return fun.apply(object, args);
	}
}

var BindAsEventListener = function(object, fun) {
	return function(event) {
		return fun.call(object, Event(event));
	}
}

function Event(e){
	var oEvent = isIE ? window.event : e;
	if (isIE) {
		oEvent.pageX = oEvent.clientX + document.documentElement.scrollLeft;
		oEvent.pageY = oEvent.clientY + document.documentElement.scrollTop;
		oEvent.preventDefault = function () { this.returnValue = false; };
		oEvent.detail = oEvent.wheelDelta / (-40);
		oEvent.stopPropagation = function(){ this.cancelBubble = true; }; 
	}
	return oEvent;
}

var CurrentStyle = function(element){
	return element.currentStyle || document.defaultView.getComputedStyle(element, null);
}

function addEventHandler(oTarget, sEventType, fnHandler) {
	if (oTarget.addEventListener) {
		oTarget.addEventListener(sEventType, fnHandler, false);
	} else if (oTarget.attachEvent) {
		oTarget.attachEvent("on" + sEventType, fnHandler);
	} else {
		oTarget["on" + sEventType] = fnHandler;
	}
};

function removeEventHandler(oTarget, sEventType, fnHandler) {
    if (oTarget.removeEventListener) {
        oTarget.removeEventListener(sEventType, fnHandler, false);
    } else if (oTarget.detachEvent) {
        oTarget.detachEvent("on" + sEventType, fnHandler);
    } else { 
        oTarget["on" + sEventType] = null;
    }
};

// 设置单选值
function setRadioValue(name, value) {
	var radios = document.getElementsByName(name);
	for (var i = 0; i < radios.length; i++) {
		if (radios[i].value == value) {
			radios[i].checked = true;
			break;
		}
	}
}

// 获取单选值
function getRadioValue(name) {
	var radios = document.getElementsByName(name);
	for (var i = 0; i < radios.length; i++) {
		if (radios[i].checked) {
			return radios[i].value;
		}
	}
	return radios[0].value;
}

// 设置下拉框值
function setSelectValue(id, value) {
	var options = O(id).getElementsByTagName("option"); ;
	for (var i = 0; i < options.length; i++) {
		if (options[i].value == value) {
			options[i].selected = true;
			break;
		}
	}
}

// 获取下拉框值
function getSelectValue(id) {
	var options = O(id).getElementsByTagName("option"); ;
	for (var i = 0; i < options.length; i++) {
		if (options[i].selected) {
			return options[i].value;
		}
	}
	return options[0].value;
}
