namespace BOTWM.Common
{
    public static class ServerError
    {
        public static string GetErrorMessage(int code)
        {
            string error = "";
            switch (code)
            {
                case 1:
                    error = "No Error";
                    break;
                case 2: 
                    error = "Unassigned PlayerNumber";
                    break;
                case 3:
                    error = "Wrong/Invalid Password";
                    break;
                case 4:
                    error = "Only whitespace/empty name";
                    break;
                case 5:
                    error = "User already exists with same name";
                    break;
                case 6:
                    error = "The name you are using is banned in this server";
                    break;
                case 7:
                    error = "Your IP is banned in this server";
                    break;
                default:
                    error = "Unknown error code";
                    break;
            }
            return error;
        }
    }
}