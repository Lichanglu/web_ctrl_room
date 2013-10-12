//滑动条程序
var Slider = Class.create();
Slider.prototype = {
  //容器对象，滑块
  initialize: function(container, bar, options) {
	this.Bar = O(bar);
	this.Container = O(container);
	this._timer = null;//自动滑移的定时器
	this._ondrag = false;//解决ie的click问题
	//是否最小值、最大值、中间值
	this._IsMin = this._IsMax = this._IsMid = false;
	//实例化一个拖放对象，并限定范围
	this._drag = new Drag(this.Bar, { Limit: true, mxContainer: this.Container,
		onStart: Bind(this, this.DragStart), onStop: Bind(this, this.DragStop), onMove: Bind(this, this.Move)
	});
	
	this.SetOptions(options);
	
	this.WheelSpeed = Math.max(0, this.options.WheelSpeed);
	this.KeySpeed = Math.max(0, this.options.KeySpeed);
	
	this.MinValue = this.options.MinValue;
	this.MaxValue = this.options.MaxValue;
	
	this.RunTime = Math.max(1, this.options.RunTime);
	this.RunStep = Math.max(1, this.options.RunStep);
	
	this.Ease = !!this.options.Ease;
	this.EaseStep = Math.max(1, this.options.EaseStep);
	
	this.onMin = this.options.onMin;
	this.onMax = this.options.onMax;
	this.onMid = this.options.onMid;
	
	this.onDragStart = this.options.onDragStart;
	this.onDragStop = this.options.onDragStop;
	
	this.onMove = this.options.onMove;
	
	this._horizontal = !!this.options.Horizontal;//一般不允许修改
	
	//锁定拖放方向
	this._drag[this._horizontal ? "LockY" : "LockX"] = true;
	
	//点击控制
	addEventHandler(this.Container, "click", BindAsEventListener(this, function(e){ this._ondrag || this.ClickCtrl(e);}));
	//取消冒泡，防止跟Container的click冲突
	addEventHandler(this.Bar, "click", BindAsEventListener(this, function(e){ e.stopPropagation(); }));
	
	//设置鼠标滚轮控制
	this.WheelBind(this.Container);
	//设置方向键控制
	this.KeyBind(this.Container);
	//修正获取焦点
	var oFocus = isIE ? (this.KeyBind(this.Bar), this.Bar) : this.Container;
	addEventHandler(this.Bar, "mousedown", function(){ oFocus.focus(); });
	//ie鼠标捕获和ff的取消默认动作都不能获得焦点，所以要手动获取
	//如果ie把focus设置到Container，那么在出现滚动条时ie的focus可能会导致自动滚屏
  },
  //设置默认属性
  SetOptions: function(options) {
	this.options = {//默认值
		MinValue:	0,//最小值
		MaxValue:	100,//最大值
		WheelSpeed: 5,//鼠标滚轮速度,越大越快(0则取消鼠标滚轮控制)
		KeySpeed: 	50,//方向键滚动速度,越大越慢(0则取消方向键控制)
		Horizontal:	true,//是否水平滑动
		RunTime:	20,//自动滑移的延时时间,越大越慢
		RunStep:	2,//自动滑移每次滑动的百分比
		Ease:		false,//是否缓动
		EaseStep:	5,//缓动等级,越大越慢
		onMin:		function(){},//最小值时执行
		onMax:		function(){},//最大值时执行
		onMid:		function(){},//中间值时执行
		onDragStart:function(){},//拖动开始时执行
		onDragStop:	function(){},//拖动结束时执行
		onMove:		function(){}//滑动时执行
	};
	Extend(this.options, options || {});
  },
  //开始拖放滑动
  DragStart: function() {
  	this.Stop();
	this.onDragStart();
	this._ondrag = true;
  },
  //结束拖放滑动
  DragStop: function() {
  	this.onDragStop();
	setTimeout(Bind(this, function(){ this._ondrag = false; }), 10);
  },
  //滑动中
  Move: function() {
  	this.onMove();
	
	var percent = this.GetPercent();
	//最小值判断
	if(percent > 0){
		this._IsMin = false;
	}else{
		if(!this._IsMin){ this.onMin(); this._IsMin = true; }
	}
	//最大值判断
	if(percent < 1){
		this._IsMax = false;
	}else{
		if(!this._IsMax){ this.onMax(); this._IsMax = true; }
	}
	//中间值判断
	if(percent > 0 && percent < 1){
		if(!this._IsMid){ this.onMid(); this._IsMid = true; }
	}else{
		this._IsMid = false;
	}
  },
  //鼠标点击控制
  ClickCtrl: function(e) {
	var o = this.Container, iLeft = o.offsetLeft, iTop = o.offsetTop;
	while (o.offsetParent) { o = o.offsetParent; iLeft += o.offsetLeft; iTop += o.offsetTop; }
	//考虑有滚动条，要用pageX和pageY
	this.EasePos(e.pageX - iLeft - this.Bar.offsetWidth / 2, e.pageY - iTop - this.Bar.offsetHeight / 2);
  },
  //鼠标滚轮控制
  WheelCtrl: function(e) {
	var i = this.WheelSpeed * e.detail;
	this.SetPos(this.Bar.offsetLeft + i, this.Bar.offsetTop + i);
	//防止触发其他滚动条
	e.preventDefault();
  },
  //绑定鼠标滚轮
  WheelBind: function(o) {
  	//鼠标滚轮控制
	addEventHandler(o, isIE ? "mousewheel" : "DOMMouseScroll", BindAsEventListener(this, this.WheelCtrl));
  },
  //方向键控制
  KeyCtrl: function(e) {
	if(this.KeySpeed){
		var iLeft = this.Bar.offsetLeft, iWidth = (this.Container.clientWidth - this.Bar.offsetWidth) / this.KeySpeed
			, iTop = this.Bar.offsetTop, iHeight = (this.Container.clientHeight - this.Bar.offsetHeight) / this.KeySpeed;
		//根据按键设置值
		switch (e.keyCode) {
			case 37 ://左
				iLeft -= iWidth; break;
			case 38 ://上
				iTop -= iHeight; break;
			case 39 ://右
				iLeft += iWidth; break;
			case 40 ://下
				iTop += iHeight; break;
			default :
				return;//不是方向按键返回
		}
		this.SetPos(iLeft, iTop);
		//防止触发其他滚动条
		e.preventDefault();
	}
  },
  //绑定方向键
  KeyBind: function(o) {
	addEventHandler(o, "keydown", BindAsEventListener(this, this.KeyCtrl));
	//设置tabIndex使设置对象能支持focus
	o.tabIndex = -1;
	//取消focus时出现的虚线框
	isIE || (o.style.outline = "none");
  },
  //获取当前值
  GetValue: function() {
	//根据最大最小值和滑动百分比取值
	return this.MinValue + this.GetPercent() * (this.MaxValue - this.MinValue);
  },
  //设置值位置
  SetValue: function(value) {
	//根据最大最小值和参数值设置滑块位置
	this.SetPercent((value- this.MinValue)/(this.MaxValue - this.MinValue));
  },
  //获取百分比
  GetPercent: function() {
	//根据滑动条滑块取百分比
	return this._horizontal ? this.Bar.offsetLeft / (this.Container.clientWidth - this.Bar.offsetWidth)
		: this.Bar.offsetTop / (this.Container.clientHeight - this.Bar.offsetHeight)
  },
  //设置百分比位置
  SetPercent: function(value) {
	//根据百分比设置滑块位置
	this.EasePos((this.Container.clientWidth - this.Bar.offsetWidth) * value, (this.Container.clientHeight - this.Bar.offsetHeight) * value);
  },
  //自动滑移(是否递增)
  Run: function(bIncrease) {
	this.Stop();
	//修正一下bIncrease
	bIncrease = !!bIncrease;
	//根据是否递增来设置值
	var percent = this.GetPercent() + (bIncrease ? 1 : -1) * this.RunStep / 100;
	this.SetPos((this.Container.clientWidth - this.Bar.offsetWidth) * percent, (this.Container.clientHeight - this.Bar.offsetHeight) * percent);
	//如果没有到极限值就继续滑移
	if(!(bIncrease ? this._IsMax : this._IsMin)){
		this._timer = setTimeout(Bind(this, this.Run, bIncrease), this.RunTime);
	}
  },
  //停止滑移
  Stop: function() {
	clearTimeout(this._timer);
  },
  //缓动滑移
  EasePos: function(iLeftT, iTopT) {
	this.Stop();
	//必须是整数，否则可能死循环
	iLeftT = Math.round(iLeftT); iTopT = Math.round(iTopT);
	//如果没有设置缓动
	if(!this.Ease){ this.SetPos(iLeftT, iTopT); return; }
	//获取缓动参数
	var iLeftN = this.Bar.offsetLeft, iLeftS = this.GetStep(iLeftT, iLeftN)
	, iTopN = this.Bar.offsetTop, iTopS = this.GetStep(iTopT, iTopN);
	//如果参数有值
	if(this._horizontal ? iLeftS : iTopS){
		//设置位置
		this.SetPos(iLeftN + iLeftS, iTopN + iTopS);
		//如果没有到极限值则继续缓动
		if(this._IsMid){ this._timer = setTimeout(Bind(this, this.EasePos, iLeftT, iTopT), this.RunTime); }
	}
  },
  //获取步长
  GetStep: function(iTarget, iNow) {
    var iStep = (iTarget - iNow) / this.EaseStep;
    if (iStep == 0) return 0;
    if (Math.abs(iStep) < 1) return (iStep > 0 ? 1 : -1);
    return iStep;
  },
  //设置滑块位置
  SetPos: function(iLeft, iTop) {
	this.Stop();
	this._drag.SetPos(iLeft, iTop);
  }
};

//拖放程序
var Drag = Class.create();
Drag.prototype = {
  //拖放对象
  initialize: function(drag, options) {
	this.Drag = O(drag);//拖放对象
	this._x = this._y = 0;//记录鼠标相对拖放对象的位置
	this._marginLeft = this._marginTop = 0;//记录margin
	//事件对象(用于绑定移除事件)
	this._fM = BindAsEventListener(this, this.Move);
	this._fS = Bind(this, this.Stop);
	
	this.SetOptions(options);
	
	this.Limit = !!this.options.Limit;
	this.mxLeft = parseInt(this.options.mxLeft);
	this.mxRight = parseInt(this.options.mxRight);
	this.mxTop = parseInt(this.options.mxTop);
	this.mxBottom = parseInt(this.options.mxBottom);
	
	this.LockX = !!this.options.LockX;
	this.LockY = !!this.options.LockY;
	this.Lock = !!this.options.Lock;
	
	this.onStart = this.options.onStart;
	this.onMove = this.options.onMove;
	this.onStop = this.options.onStop;
	
	this._Handle = O(this.options.Handle) || this.Drag;
	this._mxContainer = O(this.options.mxContainer) || null;
	
	this.Drag.style.position = "absolute";
	//透明
	if(isIE && !!this.options.Transparent){
		//填充拖放对象
		with(this._Handle.appendChild(document.createElement("div")).style){
			width = height = "100%"; backgroundColor = "#fff"; filter = "alpha(opacity:0)"; fontSize = 0;
		}
	}
	//修正范围
	this.Repair();
	addEventHandler(this._Handle, "mousedown", BindAsEventListener(this, this.Start));
  },
  //设置默认属性
  SetOptions: function(options) {
	this.options = {//默认值
		Handle:			"",//设置触发对象（不设置则使用拖放对象）
		Limit:			false,//是否设置范围限制(为true时下面参数有用,可以是负数)
		mxLeft:			0,//左边限制
		mxRight:		9999,//右边限制
		mxTop:			0,//上边限制
		mxBottom:		9999,//下边限制
		mxContainer:	"",//指定限制在容器内
		LockX:			false,//是否锁定水平方向拖放
		LockY:			false,//是否锁定垂直方向拖放
		Lock:			false,//是否锁定
		Transparent:	false,//是否透明
		onStart:		function(){},//开始移动时执行
		onMove:			function(){},//移动时执行
		onStop:			function(){}//结束移动时执行
	};
	Extend(this.options, options || {});
  },
  //准备拖动
  Start: function(oEvent) {
	if(this.Lock){ return; }
	this.Repair();
	//记录鼠标相对拖放对象的位置
	this._x = oEvent.clientX - this.Drag.offsetLeft;
	this._y = oEvent.clientY - this.Drag.offsetTop;
	//记录margin
	this._marginLeft = parseInt(CurrentStyle(this.Drag).marginLeft) || 0;
	this._marginTop = parseInt(CurrentStyle(this.Drag).marginTop) || 0;
	//mousemove时移动 mouseup时停止
	addEventHandler(document, "mousemove", this._fM);
	addEventHandler(document, "mouseup", this._fS);
	if(isIE){
		//焦点丢失
		addEventHandler(this._Handle, "losecapture", this._fS);
		//设置鼠标捕获
		this._Handle.setCapture();
	}else{
		//焦点丢失
		addEventHandler(window, "blur", this._fS);
		//阻止默认动作
		oEvent.preventDefault();
	};
	//附加程序
	this.onStart();
  },
  //修正范围
  Repair: function() {
	if(this.Limit){
		//修正错误范围参数
		this.mxRight = Math.max(this.mxRight, this.mxLeft + this.Drag.offsetWidth);
		this.mxBottom = Math.max(this.mxBottom, this.mxTop + this.Drag.offsetHeight);
		//如果有容器必须设置position为relative来相对定位，并在获取offset之前设置
		!this._mxContainer || CurrentStyle(this._mxContainer).position == "relative" || CurrentStyle(this._mxContainer).position == "absolute" || (this._mxContainer.style.position = "relative");
	}
  },
  //拖动
  Move: function(oEvent) {
	//判断是否锁定
	if(this.Lock){ this.Stop(); return; };
	//清除选择
	window.getSelection ? window.getSelection().removeAllRanges() : document.selection.empty();
	//设置移动参数
	this.SetPos(oEvent.clientX - this._x, oEvent.clientY - this._y);
  },
  //设置位置
  SetPos: function(iLeft, iTop) {
	//设置范围限制
	if(this.Limit){
		//设置范围参数
		var mxLeft = this.mxLeft, mxRight = this.mxRight, mxTop = this.mxTop, mxBottom = this.mxBottom;
		//如果设置了容器，再修正范围参数
		if(!!this._mxContainer){
			mxLeft = Math.max(mxLeft, 0);
			mxTop = Math.max(mxTop, 0);
			mxRight = Math.min(mxRight, this._mxContainer.clientWidth);
			mxBottom = Math.min(mxBottom, this._mxContainer.clientHeight);
		};
		//修正移动参数
		iLeft = Math.max(Math.min(iLeft, mxRight - this.Drag.offsetWidth), mxLeft);
		iTop = Math.max(Math.min(iTop, mxBottom - this.Drag.offsetHeight), mxTop);
	}
	//设置位置，并修正margin
	if(!this.LockX){ this.Drag.style.left = iLeft - this._marginLeft + "px"; }
	if(!this.LockY){ this.Drag.style.top = iTop - this._marginTop + "px"; }
	//附加程序
	this.onMove();
  },
  //停止拖动
  Stop: function() {
	//移除事件
	removeEventHandler(document, "mousemove", this._fM);
	removeEventHandler(document, "mouseup", this._fS);
	if(isIE){
		removeEventHandler(this._Handle, "losecapture", this._fS);
		this._Handle.releaseCapture();
	}else{
		removeEventHandler(window, "blur", this._fS);
	};
	//附加程序
	this.onStop();
  }
};