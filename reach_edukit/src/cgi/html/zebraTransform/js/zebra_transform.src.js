/**
 *  Zebra_TransForm
 *
 *  A tiny (4KB minified) jQuery plugin for styling the appearance of checkboxes, radio buttons and select boxes without
 *  sacrificing functionality and accessibility: the elements preserve their <em>tabindex</em>, give visual feedback when
 *  having the focus, can be accessed by using the keyboard, and look and behave in the same way in all major browsers.
 *
 *  Visit {@link http://stefangabos.ro/jquery/zebra-transform/} for more information.
 *
 *  For more resources visit {@link http://stefangabos.ro/}
 *
 *  @author     Stefan Gabos <contact@stefangabos.ro>
 *  @version    2.1 (last revision: February 19, 2012)
 *  @copyright  (c) 2011 - 2012 Stefan Gabos
 *  @license    http://www.gnu.org/licenses/lgpl-3.0.txt GNU LESSER GENERAL PUBLIC LICENSE
 *  @package    Zebra_TransForm
 */
;(function($) {

    $.Zebra_TransForm = function(elements, options) {

        // plugin's default options
        var defaults = {

                style_disabled_labels:  true

            },

            // to avoid confusions, use "plugin" to reference the current instance of the object
            plugin = this;

        // this will hold the merged default, and user-provided options
        plugin.settings = {}

        // the "constructor" method that gets called when the object is created
        var init = function() {

            // the plugin's final properties are the merged default and user-provided options (if any)
            plugin.settings = $.extend({}, defaults, options);

            // replace elements
            plugin.update(elements);

        }

        /**
         *  If you dynamically add or enable/disable controls, call this method to update the elements' style
         *
         *  @param  mixed    A jQuery object or a collection of jQuery objects as returned by jQuery's selector engine
         *
         *  @return void
         */
        plugin.update = function(elements)
        {

            // if invalid collection of element to replace
            if (undefined == elements || typeof elements.each != 'function')

                // replace all replaceable elements
                elements = $('input[type="checkbox"], input[type="radio"], select');

            // iterate through replaceable elements
            elements.each(function(index, element) {

                var

                    // reference to jQuery version of the element that needs to be replaced
                    $element = $(element),

                    // reference to the actual DOM element
                    element = element,

                    // get the type of the element
                    type =  $element.is('input:checkbox') ? 'checkbox' :
                            ($element.is('input:radio') ? 'radio' :
                            ($element.is('select') ? 'select' :
                            false));

                // if element is a supported type
                if (type) {

                    // make the first letter capital
                    type = type.charAt(0).toUpperCase() + type.slice(1);

                    var

                        // reference to the replacement div
                        replacement = $element.data('Zebra_TransForm_' + type),

                        // get some of the element's attributes
                        attributes = {

                            'checked':      $element.attr('checked'),
                            'disabled':     $element.attr('disabled'),
                            'multiple':     $element.attr('multiple'),
                            'size':         $element.attr('size')

                        }

                    // if element is not a list or a multi-select box
                    if (!(type == 'Select' && (attributes.multiple || attributes.size))) {

                        // if element was replaced before, remove it from the DOM
                        if (replacement) replacement.remove();// replacement.destroy();

                        var

                            // get the element's position, relative to the offset parent
                            position = $element.position(),

                            // get some of the element's CSS properties
                            styles = {

                                'width':            $element.outerWidth(),
                                'height':           $element.outerHeight(),
                                'marginLeft':       parseInt($element.css('marginLeft'), 10) || 0,
                                'marginTop':        parseInt($element.css('marginTop'), 10) || 0

                            },

                            // create the replacement div
                            replacement =

                                jQuery('<div>', {'class': 'Zebra_TransForm_' + type}).

                                // the replacement div will be invisible for now
                                css('visibility', 'hidden').

                                // save a reference to the original element
                                data(type, $element);

                        // if element is a checkbox or radio button
                        if (type != 'Select')

                            // create the tick and add it to the replacement div
                            replacement.append(jQuery('<div>', {

                                'class':    (type == 'Checkbox' ? 'Zebra_TransForm_Checkbox_Tick' : 'Zebra_TransForm_Radio_Dot')

                            // when the replacement is clicked
                            })).bind('click', function() {

                                // trigger the original element's "onChange" event
                                $(this).data(type).trigger('change');

                            });

                        // if element is a select box
                        else {

                            // create the arrow element
                            // and add it to the replacement div
                            jQuery('<div>', {'class': 'Zebra_TransForm_Arrow'}).appendTo(replacement);

                            // create the element showing the currently selected value
                            // and clone the original element's font related CSS properties
                            jQuery('<div>', {'class': 'Zebra_TransForm_Text'}).css({

                                'fontFamily':   $element.css('fontFamily'),
                                'fontSize':     $element.css('fontSize'),
                                'fontStyle':    $element.css('fontStyle'),
                                'fontWeight':   $element.css('fontWeight')

                            // add the text value of the currently selected option, and add everything to the replacement div
                            }).text(element.options[element.selectedIndex].text).appendTo(replacement);

                        }

                        // add the replacement div to the DOM, right next to the element it replaces
                        // we need to add it to the DOM now so that we can use width(), outerWidth(), etc functions later
                        replacement.insertAfter($element);

                        // if element is a checkbox or radio button
                        if (type != 'Select')

                            // place the replacement div so that it's *exactly* above the original element
                            replacement.css({

                                'left':         position.left + ((styles.width - replacement.width()) / 2) + styles.marginLeft,
                                'top':          position.top + ((styles.height - replacement.height()) / 2) + styles.marginTop

                            // style the element according to its state
                            }).addClass(

                                // is checked (but not disabled)
                                (attributes.checked && !attributes.disabled ? 'Zebra_TransForm_' + type + '_Checked' : '') +

                                // is disabled (but not checked)?
                                (attributes.disabled && !attributes.checked ? 'Zebra_TransForm_' + type + '_Disabled' : '') +

                                // is both disabled and checked
                                (attributes.disabled && attributes.checked ? ' Zebra_TransForm_' + type + '_Checked_Disabled' : '')

                            );

                        // if element is a select box
                        else {

                            // if select box is disabled, style accordingly
                            if (attributes.disabled) replacement.addClass('Zebra_TransForm_Select_Disabled');

                            // get some CSS properties
                            $.extend(styles, {

                                'paddingTop':       parseInt($element.css('paddingTop'), 10) || 0,
                                'paddingRight':     parseInt($element.css('paddingRight'), 10) || 0,
                                'paddingBottom':    parseInt($element.css('paddingBottom'), 10) || 0,
                                'paddingLeft':      parseInt($element.css('paddingLeft'), 10) || 0

                            });

                            // if select box has "position:static"
                            if ($element.css('position') == 'static')

                                // set the select box's position to "relative" (so that we can use z-index)
                                // and reset any "top", "right", "bottom" and "left" values as these are not taken
                                // into account when position is "static" but would be taken into account when
                                // position is set to "relative"
                                $element.css({

                                    'position': 'relative',
                                    'top':      '',
                                    'right':    '',
                                    'bottom':   '',
                                    'left':     ''

                                });

                            // the select box needs to be above the replacement div
                            $element.css('z-index', 20);

                            // if browser is Internet Explorer 7
                            if ($.browser.msie && $.browser.version.charAt(0) == '7') {

                                // since IE7 doesn't support paddings on select boxes, we'll emulate that by
                                // adding margins to the select box, while keeping the replacement div in the
                                // select box's original position

                                // emulate paddings by setting the margins
                                $element.css({
                                    'marginTop':    styles.paddingTop,
                                    'marginBottom': styles.paddingBottom,
                                    'marginLeft':   styles.paddingLeft,
                                    'marginRight':  styles.paddingRight
                                });

                                // the width and height of the replacement div need to be increased
                                // to accomodate padding
                                styles.width += (styles.paddingLeft + styles.paddingRight);
                                styles.height += (styles.paddingTop + styles.paddingBottom);

                            }

                            // place the replacement div so that it's *exactly* above the original element
                            replacement.css({

                                // because the replacement div doesn't have its "width" and "height" set yet,
                                // outerWidth() and outerHeight will return the size of the borders for now

                                'left':     position.left + styles.marginLeft,
                                'top':      position.top + styles.marginTop,
                                'width':    styles.width - replacement.outerWidth(),
                                'height':   styles.height - replacement.outerHeight()

                            });

                            // reference to the arrow
                            var arrow = replacement.find('.Zebra_TransForm_Arrow');

                            // position the arrow
                            arrow.css({

                                'top':      (replacement.innerHeight() - arrow.outerHeight()) / 2,
                                'right':    $.browser.webkit && !navigator.userAgent.match(/chrome/i) ? 0 : styles.paddingRight

                            });

                            // reference to the text
                            var text = replacement.find('.Zebra_TransForm_Text');

                            // position the text
                            text.css({

                                'top':      (replacement.innerHeight() - text.outerHeight()) / 2,
                                'left':     styles.paddingLeft

                            });

                        }

                        // handle events on the original element
                        $element.bind({

                            // when the element receives focus
                            'focus': function() {

                                // add a class to the replacement div
                                $(this).data('Zebra_TransForm_' + type).addClass('Zebra_TransForm_' + type + '_Focus');

                            },

                            // when the element loses focus
                            'blur': function() {

                                // remove a class from the replacement div
                                $(this).data('Zebra_TransForm_' + type).removeClass('Zebra_TransForm_' + type + '_Focus');

                            },

                            // when the original element's state changes
                            'change': function() {

                                var

                                    // reference to the jQuery version of the element
                                    $element = $(this),

                                    // reference to the actual DOM element
                                    element = $element.get(0),

                                    // reference to the replacement div
                                    replacement = $element.data('Zebra_TransForm_' + type);

                                // if element is not disabled
                                if (!$element.attr('disabled')) {

                                    // if we're doing checkboxes
                                    if (type == 'Checkbox') {

                                        // toggle a class on the replacement div
                                        replacement.toggleClass('Zebra_TransForm_Checkbox_Checked');

                                        // set the "checked" attribute accordingly
                                        if (replacement.hasClass('Zebra_TransForm_Checkbox_Checked'))

                                            $element.attr('checked', 'checked');

                                        else

                                            $element.removeAttr('checked', 'checked');

                                    // if we're doing radio buttons
                                    } else if (type == 'Radio') {

                                        // find all radio buttons sharing the name of the currently clicked
                                        // and iterate through the found elements
                                        $('input:radio[name=' + $element.attr('name') + ']').each(function(index, control) {

                                            // reference to the jQuery version of the element
                                            var $control = $(control);

                                            // remove the "checked" attribute
                                            $control.removeAttr('checked');

                                            // remove a class from the replacement div
                                            $control.data('Zebra_TransForm_Radio').removeClass('Zebra_TransForm_Radio_Checked');

                                        });

                                        // add class to replacement div of the currently clicked element
                                        replacement.addClass('Zebra_TransForm_Radio_Checked');

                                        // set the "checked" attribute of the currently clicked element
                                        // remember, we remove these in the lines above
                                        $element.attr('checked', 'checked');

                                    // if select boxes
                                    } else

                                        // put the text value in the replacement div
                                        replacement.find('.Zebra_TransForm_Text').html(element.options[element.selectedIndex].text);

                                }

                            },

                            'keyup': function(e) {

                                // if element is a select box and "up", "down" or "enter" keys are pressed
                                if (type == 'Select' && (e.which == 38 || e.which == 40 || e.which == 13)) {

                                    var

                                        // reference to the jQuery version of the element
                                        $element = $(this),

                                        // reference to the actual DOM element
                                        element = $element.get(0),

                                        // reference to the replacement div
                                        replacement = $element.data('Zebra_TransForm_' + type);

                                    // put the text value in the replacement div
                                    replacement.find('.Zebra_TransForm_Text').text(element.options[element.selectedIndex].text);

                                }

                            }

                        // make the original element *almost* invisible
                        // (if the element is visible it will be part of the tab-order and accessible by keyboard!)
                        // and save a reference to the replacement div
                        }).css('opacity', '0.0001').data('Zebra_TransForm_' + type, replacement);

                        // make the replacement div visible
                        replacement.css('visibility', 'visible');

                    // if a multi-select box or a list
                    } else if (type == 'Select' && (attributes.multiple || attributes.size))

                        // style the element according to its current state
                        $element.addClass(

                            // if a multi-select or a list
                            attributes.multiple || attributes.size ? 'Zebra_TransForm_List' : ''

                        );

                    // get the attached label
                    var label = $('label[for="' + $element.attr('id') + '"]');

                    // if an attached label exists
                    if (label)

                        //  style the label according to its current state
                        label.addClass(

                            // if element is disabled and labels attached to disabled controls are to be "disabled"
                            attributes.disabled && plugin.settings.style_disabled_labels ? 'Zebra_TransForm_Label_Disabled' : ''

                        );

                }

            });

        }

        // call the "constructor" method
        init();

    }

})(jQuery);

var zebraTransform = new jQuery.Zebra_TransForm();