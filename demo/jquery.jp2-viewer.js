// shim layer with setTimeout fallback
window.requestAnimFrame = (function(){
	return window.requestAnimationFrame       ||
		window.webkitRequestAnimationFrame ||
		window.mozRequestAnimationFrame    ||
		function( callback ){
			window.setTimeout(callback, 1000 / 60);
		};
})();


(function ( $ ) {
	$.fn.jp2Viewer = function(filename, opts) {
		var canvas = this.get(0);
		var ctx = canvas.getContext('2d');
		var bufcan = $("<canvas>").get(0);
		var buffer = bufcan.getContext('2d');
		$("body").append($(bufcan).css({"border": "1px solid"}));

		var dataType = opts.dataType || "json";
		var primary = opts.primary || "http://" + window.location.hostname + "/cgi-bin/jp2";
		var workers = opts.workers || [{address: primary, cores: 1}]; 
		var initScale = opts.initScale || "full-width";

		var jp2Header = false;
		var tiles = {};

		var scale;
		var reduction;
		var reductionScale;
		var currentWorker = 0;
		var xPos = 0;
		var yPos = 0;
		var rotation = 0;
		var curX;
		var curY;
		var oldX;
		var oldY;
		var dragging = false;
		var loadTrigger = false;
		var showTrigger = false;
		var incompleteTiles = [];

		bufcan.width = canvas.width;
		bufcan.height = canvas.height;
		buffer.fillStyle = opts.backgroundColor || "#aaa";


		this.on("mousedown", function(e) {
			dragging = true;
			curX = e.clientX;
			curY = e.clientY;
			oldX = xPos;
			oldY = yPos;
		});

		this.on("mousemove", function(e) {
			if(dragging) {
				var movX = curX - e.clientX;
				var movY = curY - e.clientY;
				switch(rotation) {
					case 90: var swp = movX; movX = movY; movY = -swp; break;
					case 180: movX = -movX; movY = -movY; break;
					case 270: var swp = movX; movX = -movY; movY = swp; break;
					default:
				}

				xPos -= movX;
				yPos -= movY;
				ensureBounds();
				showImage();
				if(oldX != xPos || oldY != yPos) { loadTrigger = true; }
				curX = e.clientX;
				curY = e.clientY;
			}
		});

		this.on("mouseup mouseout", function(e) {
			if(oldX != xPos || oldY != yPos) { loadTrigger = true; }
			dragging = false;
		});

		this.on("mousewheel", function(e, delta, deltaX, deltaY) {
			if(delta > 0) {
				initScale = scale * 1.1;
				initialize();
			} else if(delta < 0) {
				initScale = scale * 0.9;
				initialize();
			}
		});

		this.on("setrotation", function(e, rot) {
			rotation = rot;
			if(rotation == 90 || rotation == 270) {
				bufcan.width = canvas.height;
				bufcan.height = canvas.width;
			} else {
				bufcan.width = canvas.width;
				bufcan.height = canvas.height;
			}
			ensureBounds();
			loadImage();
		});

		function ensureBounds() {
			if(xPos + (jp2Header.x1 * scale) <= bufcan.width) { xPos = bufcan.width - (jp2Header.x1 * scale); }
			if(yPos + (jp2Header.y1 * scale) <= bufcan.height) { yPos = bufcan.height - (jp2Header.y1 * scale); }
			if(xPos > 0) { xPos = 0; }
			if(yPos > 0) { yPos = 0; }
			if(jp2Header.x1 * scale <= bufcan.width) { 
				xPos = Math.floor((bufcan.width - jp2Header.x1 * scale) / 2);
			}
		}

		function reduce(val, reduction) {
			if(reduction == 0) { return val; }
			return reduce(val / 2, --reduction);
		}

		function setScale(s) {
			switch(s) {
				case "full-width": scale =  canvas.width / jp2Header.x1; break;
				case "full-height": scale = canvas.height / jp2Header.y1; break;
				default: 
					scale = parseFloat(s);
					if(scale == 0.0) { scale = 1.0; }
			}
		}

		function setReduction(s) {
			var s = s || scale;
			reduction = jp2Header.num_res;
			while(--reduction) {
				if(reduce(1.0, reduction) > s) { 
					break; 
				}
			}
		}

		function initializeTiles() {
			for(var i = 0; i < jp2Header.num_res; i++) {
				tiles["redux-" + i] = [];
			}
		}

		function drawTile(tile) {
			if(tile.dims.r != reduction) { return; }
			if(tile.img.complete) {
				buffer.drawImage(
					tile.img, tile.dims.x, tile.dims.y, 
					Math.ceil(tile.img.width * tile.dims.s), 
					Math.ceil(tile.img.height * tile.dims.s));
			} else {
				buffer.fillRect(tile.dims.x, tile.dims.y, jp2Header.tdx * tile.dims.s, jp2Header.tdy * tile.dims.s);
				incompleteTiles.push(tile);
			}
		}

		function clearSurroundings(xCorrection, yCorrection) {
			if(jp2Header.x1 * scale <= canvas.width) { 
				ctx.clearRect(xCorrection || 0, yCorrection ||	 0, xPos, canvas.height);
				ctx.clearRect(canvas.width - xPos - 1 + (xCorrection || 0), (yCorrection || 0), xPos + 1, canvas.height);
			}

			if(jp2Header.y1 * scale <= canvas.height) { 
				ctx.clearRect((xCorrection || 0), (yCorrection || 0) + jp2Header.y1 * scale, canvas.width, canvas.height - jp2Header.y1 * scale);
			}
		}

		function loadImage(_hidden) {
			var tileS = scale / reduce(1.0, reduction);
			var ch = canvas.height;
			var cw = canvas.width;
			if(rotation == 90 || rotation == 270) { var swp = cw; cw = ch; ch = swp; }
			var tilesX = Math.ceil(cw / (jp2Header.tdx * scale)) + 1;
			var tilesY = Math.ceil(ch / (jp2Header.tdy * scale)) + 1;
			var tileX = Math.floor(-xPos / (jp2Header.tdx * scale));
			var tileY = Math.floor(-yPos / (jp2Header.tdy * scale));

			if(tileX < 0) { tileX = 0; }
			if(tileY < 0) { tileY = 0; }

			if(tileX + tilesX > jp2Header.tw)  { tilesX = jp2Header.tw - tileX; }
			if(tileY + tilesY > jp2Header.th)  { tilesY = jp2Header.th - tileY; }

			for(var x = tileX; x < tileX + tilesX; x++) {
				for(var y = tileY; y < tileY + tilesY; y++) {
					var tileIndex = x + (y * jp2Header.tw);

					if(tiles["redux-" + reduction][tileIndex]) {
						tiles["redux-" + reduction][tileIndex].dims = { 
							x: xPos + Math.floor(x * (jp2Header.tdx * scale)), 
							y: yPos + Math.floor(y * (jp2Header.tdy * scale)),
							tx: x, 
							ty: y,
							s: tileS,
							r: reduction
						};
						drawTile(tiles["redux-" + reduction][tileIndex]);
					} else {
						var tile = { 
							img: new Image(),
							dims:  { 
								x: xPos + Math.floor(x * (jp2Header.tdx * scale)), 
								y: yPos + Math.floor(y * (jp2Header.tdy * scale)),
								tx: x, 
								ty: y,
								s: tileS,
								r: reduction
							}
						}
						tiles["redux-" + reduction][tileIndex] = tile;
						tile.img.src = workers[currentWorker].address + "?" + $.param({
							f: filename,
							t: tileIndex,
							r: reduction
						});
						drawTile(tile);
					}

					if(++currentWorker == workers.length) { currentWorker = 0; }
				}
			}
			showImage();
		}

		function initialize(__init) {
			jp2Header = __init || jp2Header;
			setScale(initScale);
			setReduction();
			if(__init) { initializeTiles(); }
			ensureBounds();
			loadImage();
		}

		function showImage() {
			ctx.save();
			ctx.translate(canvas.width / 2, canvas.height / 2);
			ctx.rotate(rotation * (Math.PI / 180));
			ctx.drawImage(bufcan, -bufcan.width / 2, -bufcan.height / 2);
			clearSurroundings(-bufcan.width / 2, -bufcan.height / 2);

			ctx.restore();
		}

		function drawIncompleteTiles() {
			var i = incompleteTiles.length;
			if(i == 0) { return; }
			while(--i > -1) {
				if(incompleteTiles[i].img.complete) {
					incompleteTiles[i].dims.x = xPos + Math.floor(incompleteTiles[i].dims.tx * (jp2Header.tdx * scale));
					incompleteTiles[i].dims.y = yPos + Math.floor(incompleteTiles[i].dims.ty * (jp2Header.tdy * scale)),
					incompleteTiles[i].dims.s = scale / reduce(1.0, reduction);

					drawTile(incompleteTiles[i]);
					incompleteTiles.splice(i, 1);
				}
			}
			showImage();
		}

		function preloadHiddenTiles() {
			for(var x = 0; x < jp2Header.tw; x++) {
				for(var y = 0; y < jp2Header.th; y++) {
					var tileIndex = x + (y * jp2Header.tw);
					if(!tiles["redux-" + reduction][tileIndex]) {
						tiles["redux-" + reduction][tileIndex] = { img: new Image() };
						tiles["redux-" + reduction][tileIndex].img.src = workers[currentWorker].address + "?" + $.param({
							f: filename,
							t: tileIndex,
							r: reduction
						});
					}
				}
			}
		}

		function monitorTiles() {
			requestAnimFrame(monitorTiles);
			if(!jp2Header) { return; }
			if(loadTrigger) { loadImage(); loadTrigger = false; }
			drawIncompleteTiles();
			if(incompleteTiles.length == 0) {
				preloadHiddenTiles();
			}
		}

		monitorTiles();

		$.ajax(primary, {
			data: { f: filename },
			dataType: opts.dataType,
			success: initialize
		});
	};
}( jQuery ));
