﻿using Microsoft.UI.Xaml.Data;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Microsoft.Midi.Settings.Helpers
{
    public partial class MidiFunctionBlockDirectionConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            if (value is null)
            {
                return string.Empty;
            }

            if (value is MidiFunctionBlockDirection)
            {
                var val = (MidiFunctionBlockDirection)value;

                // TODO: Localize

                switch (val)
                {
                    case MidiFunctionBlockDirection.BlockInput:
                        return "Message Destination";

                    case MidiFunctionBlockDirection.BlockOutput:
                        return "Message Source";

                    default:
                        return "Unknown";
                }
            }
            else
            {
                return string.Empty;
            }

        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }

}