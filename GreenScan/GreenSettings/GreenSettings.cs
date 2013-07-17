﻿using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls;
using System.Reflection;
using System.IO;
using System.Globalization;
using System.Windows;

namespace Green.Settings
{
    public abstract class Setting : INotifyPropertyChanged
    {
        public static CultureInfo Culture;
        static Setting()
        {
            Culture = new CultureInfo("en-US");
        }

        public Setting(string name)
        {
            Name = name;
        }

        public string Name { get; protected set; }
        
        [Flags]
        public enum Types : uint { 
            Unknown = 0, 
            Boolean = 1, 
            Enum = 2, 
            SByte = 4, 
            Int16 = 8, 
            Int32 = 16, 
            Int64 = 32, 
            Byte = 64, 
            UInt16 = 128, 
            UInt32 = 256, 
            UInt64 = 512, 
            Single = 1024, 
            Double = 2048, 
            Matrix = 4096, 
            String = 8192, 
            Path = 16384, 
            Rectangle = 32768, 
            Numeric = 4092, 
            Integer = 1020 
        };
        public Types Type { get; protected set; }
        
        public event PropertyChangedEventHandler PropertyChanged;
        public event EventHandler ValueChanged;
        protected void OnPropertyChanged(string name)
        {
            if (PropertyChanged != null) PropertyChanged(this, new PropertyChangedEventArgs(name));
        }
        protected virtual void OnValueChanged()
        {
            OnPropertyChanged("Value");
            OnPropertyChanged("StringValue");
            OnPropertyChanged("HasDefaultValue");
            if (ValueChanged != null) ValueChanged(this, null);
        }

        public abstract string StringValue { get; set; }
        public abstract bool HasDefaultValue { get; }
        public abstract void ResetValue();

        private string friendlyName = null;
        public string FriendlyName
        {
            get
            {
                if (friendlyName == null) return Name;
                else return friendlyName;
            }
            set
            {
                friendlyName = value;
            }
        }

        private bool isHidden;
        public bool IsHidden
        {
            get { return isHidden; }
            set
            {
                isHidden = value;
                OnPropertyChanged("IsHidden");
            }
        }
    }

    public class MatrixSetting : Setting
    {
        public float[,] DefaultValue { get; private set; }
        private float[,] value;
        public float[,] Value
        {
            get { return value; }
            set
            {
                this.value = value;
                OnValueChanged();
            }
        }

        public float this[int x, int y]
        {
            get
            {
                return value[x, y];
            }
            set
            {
                this.value[x, y] = value;
                OnValueChanged();
            }
        }

        public override string StringValue
        {
            get
            {
                string s = "";
                for (int r = 0; r < Rows; r++)
                {
                    if (r != 0) s += "|";
                    for (int c = 0; c < Columns; c++)
                    {
                        if (c != 0) s += ";";
                        s += value[r, c].ToString(Culture);
                    }
                }
                return s;
            }
            set
            {
                try
                {
                    string[] lines = value.Split('|');
                    float[,] newValue = new float[Rows, Columns];
                    for (int r = 0; r < Rows; r++)
                    {
                        string[] line = lines[r].Split(';');
                        for (int c = 0; c < Columns; c++)
                        {
                            newValue[r, c] = float.Parse(line[c], Culture);
                        }
                    }
                    Value = newValue;
                }
                catch { }
            }
        }

        public int Rows { get; private set; }
        public int Columns { get; private set; }
        public MatrixSetting(string name, float[,] value)
            : base(name)
        {
            Type = Types.Matrix;
            Rows = value.GetLength(0);
            Columns = value.GetLength(1);
            this.value = value;
            DefaultValue = new float[Rows, Columns];
            for (int r = 0; r < Rows; r++)
                for (int c = 0; c < Columns; c++)
                    DefaultValue[r, c] = value[r, c];
        }

        public override bool HasDefaultValue
        {
            get
            {
                for (int r = 0; r < Rows; r++)
                    for (int c = 0; c < Columns; c++)
                        if (DefaultValue[r, c] != value[r, c]) return false;
                return true;
            }
        }

        public override void ResetValue()
        {
            for (int r = 0; r < Rows; r++)
                for (int c = 0; c < Columns; c++)
                    value[r, c] = DefaultValue[r, c];
            OnValueChanged();
        }
    }

    public abstract class NumericSetting : Setting 
    {
        public int Decimals { get; protected set; }
        public NumericSetting(string name) : base(name) { }
    }

    public class NumericSetting<T> : NumericSetting where T : struct
    {
        private T value;
        public T Value 
        {
            get { return value; }
            set
            {
                if ((int)typeof(T).InvokeMember("CompareTo", BindingFlags.InvokeMethod | BindingFlags.Public | BindingFlags.Instance, null, value, new object[] { Maximum }) > 0)
                    this.value = Maximum;
                else if ((int)typeof(T).InvokeMember("CompareTo", BindingFlags.InvokeMethod | BindingFlags.Public | BindingFlags.Instance, null, value, new object[] { Minimum }) < 0)
                    this.value = Minimum;
                else
                    this.value = value;
                OnValueChanged();
            }
        }
        public T Maximum { get; private set; }
        public T Minimum { get; private set; }
        public T DefaultValue { get; private set; }        
        
        public NumericSetting(string name, T value, T min, T max, int decimals = 0) : base(name)
        {
            Types type;
            if (Enum.TryParse<Types>(typeof(T).Name, out type) && Types.Numeric.HasFlag(type))
                Type = type;
            else
                throw new NotSupportedException("The given datatype is not supported!");

            Maximum = max;
            Minimum = min;
            DefaultValue = Value = value;
            Decimals = decimals;
        }

        public override string StringValue
        {
            get
            {
                return (string)typeof(T).InvokeMember("ToString", BindingFlags.InvokeMethod | BindingFlags.Public | BindingFlags.Instance, null, value, new object[] { "F" + Decimals, Culture });
            }
            set
            {
                try
                {
                    Value = (T)typeof(T).InvokeMember("Parse", BindingFlags.InvokeMethod | BindingFlags.Static | BindingFlags.Public, null, null, new object[] { value, Culture });
                }
                catch { }
            }
        }

        public override bool HasDefaultValue
        {
            get { return DefaultValue.Equals(value); }
        }

        public override void ResetValue()
        {
            Value = DefaultValue;
        }
    }

    public class StringSetting : Setting
    {
        public char[] InvalidChars { get; private set; }
        public string DefaultValue { get; private set; }
        private string value;
        public string Value
        {
            get { return value; }
            set 
            {
                if (InvalidChars != null)
                {
                    for (int i = 0; i < value.Length; i++)
                    {
                        if (InvalidChars.Contains(value[i]))
                            return;
                    }
                }
                this.value = value;
                OnValueChanged();
            }
        }

        public override string StringValue
        {
            get { return value; }
            set { Value = value; }
        }

        public override bool HasDefaultValue
        {
            get { return DefaultValue == value; }
        }

        public override void ResetValue()
        {
            Value = DefaultValue;
        }

        public StringSetting(string name, string value, char[] invalidChars = null) : base(name)
        {
            DefaultValue = Value = value;
            Type = Types.String;
            InvalidChars = invalidChars;
        }
    }

    public class PathSetting : StringSetting
    {
        public PathSetting(string name, string value)
            : base(name, value, Path.GetInvalidPathChars())
        {
            Type = Types.Path;
        }

        public string AbsolutePath
        {
            get
            {
                if (Path.IsPathRooted(Value))
                    return Value;
                else
                {
                    string cd = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
                    if (!cd.EndsWith("\\")) cd += "\\";
                    string value = Value;
                    if (value.StartsWith("\\")) value = Value.Substring(1);
                    return cd + value;
                }
            }
        }

        public bool Exists
        {
            get
            {
                return Directory.Exists(AbsolutePath);
            }
        }
    }

    public class RectangleSetting : Setting
    {
        private Rect value;
        public Rect Value
        {
            get { return value; }
            set
            {
                this.value = value;
                OnValueChanged();
            }
        }

        public Rect DefaultValue { get; private set; }

        public RectangleSetting(string name, Rect value)
            : base(name)
        {
            Type = Types.Boolean;
            DefaultValue = Value = value;
        }

        public override string StringValue
        {
            get
            {
                return String.Format(Culture, "{0};{1};{2};{3}", new object[] { Value.X, Value.Y, Value.Width, Value.Height });
            }
            set
            {
                string[] items = value.Split(';');
                try
                {
                    Value = new Rect(
                        double.Parse(items[0], Culture),
                        double.Parse(items[1], Culture),
                        double.Parse(items[2], Culture),
                        double.Parse(items[3], Culture)
                        );

                }
                catch { }
            }
        }

        public override bool HasDefaultValue
        {
            get { return DefaultValue == Value; }
        }

        public override void ResetValue()
        {
            DefaultValue = Value;
        }
    }

    public class BooleanSetting : Setting
    {
        private bool value;
        public bool Value
        {
            get { return value; }
            set
            {
                this.value = value;
                OnValueChanged();
            }
        }

        public bool DefaultValue { get; private set; }

        public BooleanSetting(string name, bool value) : base(name)
        {
            Type = Types.Boolean;
            DefaultValue=Value = value;
        }

        public override string StringValue
        {
            get { return value.ToString(); }
            set
            {
                bool newValue;
                if (Boolean.TryParse(value, out newValue))
                {
                    Value = newValue;
                }
            }
        }

        public override bool HasDefaultValue
        {
            get { return DefaultValue == value; }
        }

        public override void ResetValue()
        {
            Value = DefaultValue;
        }
    }

    public abstract class EnumSetting : Setting
    {
        public EnumSetting(string name) : base(name) { }
        public string[] StringOptions { get; protected set; }

        private string[] friendlyOptions = null;
        public string[] FriendlyOptions
        {
            get
            {
                if (friendlyOptions == null) return StringOptions;
                else return friendlyOptions;
            }
            set
            {
                if (value.Length == StringOptions.Length)
                    friendlyOptions = value;
            }
        }

        public int SelectedIndex
        {
            get
            {
                string strval = StringValue;
                for (int i = 0; i < StringOptions.Length; i++)
                {
                    if (StringOptions[i] == strval)
                    {
                        return i;
                    }
                }
                return -1;
            }
            set
            {
                if (value > 0 && value < StringOptions.Length)
                    StringValue = StringOptions[value];
            }
        }

        public string FriendlyValue
        {
            get
            {
                return FriendlyOptions[SelectedIndex];
            }
            set
            {
                for (int i = 0; i < FriendlyOptions.Length; i++)
                {
                    if (FriendlyOptions[i] == value)
                    {
                        StringValue = StringOptions[i];
                        break;
                    }
                }
            }
        }
        protected override void OnValueChanged()
        {
            OnPropertyChanged("FriendlyValue");
            OnPropertyChanged("SelectedIndex");
            base.OnValueChanged();
        }
    }

    public class EnumSetting<T> : EnumSetting where T : struct
    {
        private T value;
        public T Value 
        {
            get { return value; }
            set { 
                this.value = value;
                OnValueChanged();
            }
        }
        public T DefaultValue { get; private set; }

        public override string StringValue
        {
            get { return value.ToString(); }
            set
            {
                T newValue;
                if(Enum.TryParse<T>(value, out newValue))
                {
                    Value = newValue;
                }
            }
        }

        public EnumSetting(string name, T value) : base(name)
        {
            if (!typeof(T).IsEnum) throw new NotSupportedException("The given datatype is not an enumeration!");
            Type = Types.Enum;
            DefaultValue = Value = value;
            StringOptions = Enum.GetNames(typeof(T));
        }

        public override bool HasDefaultValue
        {
            get { return DefaultValue.Equals(value); }
        }

        public override void ResetValue()
        {
            Value = DefaultValue;
        }
    }

    public class SettingGroup : INotifyPropertyChanged
    {
        private bool isHidden;
        public bool IsHidden
        {
            get { return isHidden; }
            set
            {
                isHidden = value;
                OnPropertyChanged("IsHidden");
            }
        }

        private bool hasDefaultValues;
        public bool HasDefaultValues 
        {
            get { return hasDefaultValues; }
            private set
            {
                hasDefaultValues = value;
                OnPropertyChanged("HasDefaultValues");
            }
        }
        public string Name { get; private set; }
        public ObservableCollection<Setting> Settings { get; private set; }
        public SettingGroup(string name)
        {
            Name = name;
            Settings = new ObservableCollection<Setting>();
            Settings.CollectionChanged += Settings_CollectionChanged;
            HasDefaultValues = true;
        }

        void Settings_CollectionChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
        {
            foreach (Setting s in e.NewItems)
            {
                s.PropertyChanged += Setting_PropertyChanged;
                s.ValueChanged += Setting_ValueChanged;
                if (!s.HasDefaultValue) HasDefaultValues = false;
            }
        }

        public event EventHandler ValueChanged;
        void Setting_ValueChanged(object sender, EventArgs e)
        {
            if (ValueChanged != null) ValueChanged(this, new EventArgs());
        }

        void Setting_PropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            bool hdv = true;
            foreach (Setting s in Settings)
            {
                if (!s.HasDefaultValue)
                {
                    hdv = false;
                    break;
                }
            }
            HasDefaultValues = hdv;
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string name)
        {
            if (PropertyChanged != null) PropertyChanged(this, new PropertyChangedEventArgs(name));
        }

        private string friendlyName = null;
        public string FriendlyName
        {
            get
            {
                if (friendlyName == null) return Name;
                else return friendlyName;
            }
            set
            {
                friendlyName = value;
            }
        }

        public void ResetToDefault()
        {
            foreach (Setting s in Settings) s.ResetValue();
        }
    }

    public class SettingManager
    {
        public ObservableCollection<SettingGroup> SettingGroups { get; private set; }
        public SettingManager()
        {
            SettingGroups = new ObservableCollection<SettingGroup>();
        }

        public bool Save(string path)
        {
            try
            {
                using (StreamWriter SW = new StreamWriter(path))
                {
                    foreach (SettingGroup SG in SettingGroups)
                    {
                        SW.WriteLine('[' + SG.Name + ']');
                        foreach (Setting S in SG.Settings)
                        {
                            SW.WriteLine(S.Name + '=' + S.StringValue);
                        }
                        SW.WriteLine();
                    }
                    SW.Close();
                }
                return true;
            }
            catch 
            {
                return false;
            }
        }

        public bool Load(string path)
        {
            try
            {
                Dictionary<string, Setting> settings = new Dictionary<string, Setting>();
                foreach (SettingGroup SG in SettingGroups)
                    foreach (Setting S in SG.Settings)
                    {
                        settings.Add(SG.Name + '.' + S.Name, S);
                    }

                string group = "", setting, value;
                string[] items;
                using (StreamReader SR = new StreamReader(path))
                {
                    while (!SR.EndOfStream)
                    {       
                        string line = SR.ReadLine();
                        if (line.Length == 0) continue;
                        if (line[0] == '[')
                        {
                            group = line.Substring(1, line.Length - 2);
                        }
                        else
                        {
                            items = line.Split('=');
                            setting = group+'.'+items[0];
                            value = items[1];
                            if (settings.ContainsKey(setting))
                            {
                                settings[setting].StringValue = value;
                            }
                        }
                    }
                    SR.Close();
                }
                return true;
            }
            catch
            {
                return false;
            }
        }
    }
}
